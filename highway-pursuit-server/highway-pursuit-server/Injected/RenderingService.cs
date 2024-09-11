using HighwayPursuitServer.Data;
using HighwayPursuitServer.Server;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    class RenderingService
    {
        private const float FULL_ZOOM = 10.0f;
        private readonly IHookManager _hookManager;
        public IntPtr Device => GetDevice();
        private IntPtr pSurface = IntPtr.Zero;
        private D3DDISPLAYMODE _currentSurfaceDisplayMode = new D3DDISPLAYMODE()
        {
            Width = 0,
            Height = 0,
            RefreshRate = 0,
            Format = D3DFORMAT.D3DFMT_UNKNOWN
        };


        public RenderingService(IHookManager hookManager)
        {
            this._hookManager = hookManager;
            this.RegisterHooks();
        }

        ~RenderingService()
        {
            if (pSurface != IntPtr.Zero)
            {
                HandleDRDERR(IDirect3DSurface8.Release(pSurface));
            }
        }

        private void HandleDRDERR(uint errorCode)
        {
            if (errorCode > 0)
            {
                throw new D3DERR(errorCode);
            }
        }

        public D3DDISPLAYMODE GetDisplayMode()
        {
            D3DDISPLAYMODE displayMode = new D3DDISPLAYMODE();
            HandleDRDERR(IDirect3DDevice8.GetDisplayMode(Device, ref displayMode));
            return displayMode;
        }

        public void SetFullscreenFlag(bool useFullscreen)
        {
            IntPtr module = _hookManager.GetModuleBase();
            IntPtr fullscreenFlag = new IntPtr(module.ToInt32() + MemoryAdresses.FULLSCREEN_FLAG_OFFSET);
            Marshal.WriteByte(fullscreenFlag, useFullscreen ? (byte)0x1 : (byte)0x0);
        }

        public void ResetZoomLevel()
        {
            IntPtr module = _hookManager.GetModuleBase();
            IntPtr zoomAnimValue = new IntPtr(module.ToInt32() + MemoryAdresses.CAMERA_ZOOM_ANIM_OFFSET);

            // Write float value into the camera zoom value offset
            byte[] bytes = BitConverter.GetBytes(FULL_ZOOM);
            Marshal.Copy(bytes, 0, zoomAnimValue, bytes.Length);
        }

        private void EnsureSurface()
        {
            // Get display mode
            var displayMode = GetDisplayMode();

            // Flag about creating a new surface
            bool surfaceInitialized = (pSurface != IntPtr.Zero);
            bool shouldUpdateSurface = (!surfaceInitialized) || (!_currentSurfaceDisplayMode.IsSameAs(displayMode));

            // Release the current surface
            if (shouldUpdateSurface && surfaceInitialized)
            {
                HandleDRDERR(IDirect3DSurface8.Release(pSurface));
            }

            // Create surface if necessary
            if (shouldUpdateSurface)
            {
                uint pSurfaceValue = 0;
                HandleDRDERR(IDirect3DDevice8.CreateImageSurface(Device, displayMode.Width, displayMode.Height, (uint)displayMode.Format, ref pSurfaceValue));
                pSurface = new IntPtr(pSurfaceValue);
                _currentSurfaceDisplayMode = displayMode;
            }
        }

        public void Screenshot(Action<IntPtr> pixelDataHandler)
        {
            EnsureSurface();

            // Back buffer method
            uint pBackBufferSurfaceValue = 0;
            HandleDRDERR(IDirect3DDevice8.GetBackBuffer(Device, 0, D3DBACKBUFFER_TYPE.TYPE_MONO, ref pBackBufferSurfaceValue));
            IntPtr pBackBufferSurface = new IntPtr(pBackBufferSurfaceValue);

            // TODO: This is leaking memory and I don't know why!
            HandleDRDERR(IDirect3DDevice8.CopyRects(Device, pBackBufferSurface, IntPtr.Zero, 0, pSurface, IntPtr.Zero));
            try
            {
                // Lock pixels
                HandleDRDERR(IDirect3DSurface8.LockRect(pSurface, out D3DLOCKED_RECT lockedRect, IntPtr.Zero, (ulong)LOCK_RECT_FLAGS.D3DLOCK_READONLY));
                // Handle pixel data
                pixelDataHandler(lockedRect.pBits);
            }
            finally
            {
                // Release resources
                HandleDRDERR(IDirect3DSurface8.UnlockRect(pSurface));
                HandleDRDERR(IDirect3DSurface8.Release(pBackBufferSurface));
            }
        }

        #region Hooking
        private void RegisterHooks()
        {
            var moduleBase = _hookManager.GetModuleBase();
            // Window Procedure
            IntPtr windowProcPtr = new IntPtr(moduleBase.ToInt32() + MemoryAdresses.WINDOW_PROC_OFFSET);
            WindowProcedure = Marshal.GetDelegateForFunctionPointer<WindowProcedure_delegate>(windowProcPtr);
            _hookManager.RegisterHook(windowProcPtr, new WindowProcedure_delegate(WindowProcedureHook));

            // D3D8 functions
            var d3d8 = _hookManager.GetD3D8Base();
            // Get display mode
            IntPtr getDisplayModePtr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.GET_DISPLAY_MODE_OFFSET);
            IDirect3DDevice8.GetDisplayMode = Marshal.GetDelegateForFunctionPointer<GetDisplayMode_delegate>(getDisplayModePtr);

            // Create image surface
            IntPtr createImageSurfacePtr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.CREATE_SURFACE_IMAGE_OFFSET);
            IDirect3DDevice8.CreateImageSurface = Marshal.GetDelegateForFunctionPointer<CreateImageSurface_delegate>(createImageSurfacePtr);

            // Get back buffer
            IntPtr getBackBufferPtr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.GET_BACK_BUFFER_OFFSET);
            IDirect3DDevice8.GetBackBuffer = Marshal.GetDelegateForFunctionPointer<GetBackBuffer_delegate>(getBackBufferPtr);

            // Copy rects
            IntPtr copyRectsPtr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.COPY_RECTS_OFFSET);
            IDirect3DDevice8.CopyRects = Marshal.GetDelegateForFunctionPointer<CopyRects_delegate>(copyRectsPtr);

            // Lock rect
            IntPtr lockRectPtr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.LOCK_RECT_OFFSET);
            IDirect3DSurface8.LockRect = Marshal.GetDelegateForFunctionPointer<LockRect_delegate>(lockRectPtr);

            // Unlock rect
            IntPtr unlockRectPtr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.UNLOCK_RECT_OFFSET);
            IDirect3DSurface8.UnlockRect = Marshal.GetDelegateForFunctionPointer<UnlockRect_delegate>(unlockRectPtr);

            // Release
            IntPtr surfaceReleasePtr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.SURFACE_RELEASE_OFFSET);
            IDirect3DSurface8.Release = Marshal.GetDelegateForFunctionPointer<Release_delegate>(surfaceReleasePtr);
        }

        private IntPtr GetDevice()
        {
            return new IntPtr(Marshal.ReadInt32(new IntPtr(_hookManager.GetModuleBase().ToInt32() + MemoryAdresses.DEVICE_PTR_OFFSET)));
        }

        #region hooks
        void WindowProcedureHook(IntPtr hwnd, uint uMsg, uint wParam, uint lParam)
        {
            const uint focused = 1;
            const uint WM_ACTIVATEAPP = 0x1c;
            const uint WM_ACTIVATE = 0x6;
            // Disable the "lose focus" callback to disable pausing
            if (uMsg == WM_ACTIVATEAPP || uMsg == WM_ACTIVATE)
            {
                WindowProcedure(hwnd, uMsg, focused, lParam);
            }
            else
            {
                WindowProcedure(hwnd, uMsg, wParam, lParam);
            }
        }
        #endregion

        #region window management functions
        static WindowProcedure_delegate WindowProcedure;
        #endregion
        #region D3D8 Functions
        static class IDirect3DDevice8
        {
            public static GetDisplayMode_delegate GetDisplayMode;
            public static CreateImageSurface_delegate CreateImageSurface;
            public static GetBackBuffer_delegate GetBackBuffer;
            public static CopyRects_delegate CopyRects;
        }

        static class IDirect3DSurface8
        {
            public static LockRect_delegate LockRect;
            public static UnlockRect_delegate UnlockRect;
            public static Release_delegate Release;
        }
        #endregion

        #region delegates        
        #region
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void WindowProcedure_delegate(IntPtr hwnd, uint uMsg, uint wParam, uint lParam);
        #endregion
        #region device methods
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint GetDisplayMode_delegate(IntPtr pDevice, ref D3DDISPLAYMODE pMode);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint CreateImageSurface_delegate(IntPtr pDevice, uint width, uint height, uint format, ref uint pSurface);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint GetBackBuffer_delegate(IntPtr pDevice, uint backBuffer, D3DBACKBUFFER_TYPE type, ref uint pSurface);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint CopyRects_delegate(IntPtr pDevice, IntPtr pSourceSurface, IntPtr pSourceRectsArray, uint cRects, IntPtr pDestinationSurface, IntPtr pDestPointsArray);
        #endregion
        #region Surface methods
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint LockRect_delegate(IntPtr pSurface, out D3DLOCKED_RECT pLockedRect, IntPtr pRect, ulong flags);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint UnlockRect_delegate(IntPtr pSurface);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint Release_delegate(IntPtr pSurface);
        #endregion
        #endregion
        #endregion
    }
}
