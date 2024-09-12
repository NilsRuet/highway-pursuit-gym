using HighwayPursuitServer.Data;
using HighwayPursuitServer.Exceptions;
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
    public struct BufferFormat
    {
        public static uint FormatToChannels(D3DFORMAT format)
        {
            switch (format)
            {
                case D3DFORMAT.D3DFMT_X8R8G8B8:
                    return 4;
                default:
                    throw new HighwayPursuitException(ErrorCode.UNSUPPORTED_BACKBUFFER_FORMAT);
            }
        }

        public uint width;
        public uint height;
        public uint channels;

        public BufferFormat(D3DSURFACE_DESC surface)
        {
            width = surface.Width;
            height = surface.Height;
            channels = FormatToChannels(surface.Format);
        }

        // Returns size in bytes
        public uint Size()
        {
            return width * height * channels;
        }
    }

    class RenderingService
    {
        private const float FULL_ZOOM = 10.0f;
        private readonly ServerParams.RenderParams _renderParams;
        private readonly IHookManager _hookManager;
        public IntPtr Device => GetDevice();

        public RenderingService(IHookManager hookManager, ServerParams.RenderParams renderParams)
        {
            this._renderParams = renderParams;
            this._hookManager = hookManager;
            this.RegisterHooks();
        }

        private void HandleDRDERR(uint errorCode)
        {
            if (errorCode > 0)
            {
                throw new D3DERR(errorCode);
            }
        }

        public BufferFormat GetBufferFormat()
        {
            BufferFormat res;

            HandleDRDERR(IDirect3DDevice8.GetBackBuffer(Device, 0, D3DBACKBUFFER_TYPE.TYPE_MONO, out var pBackBufferSurfaceValue));
            IntPtr pBackBufferSurface = new IntPtr(pBackBufferSurfaceValue);
            try
            {
                res = GetBufferFormatFromSurface(pBackBufferSurface);
            }
            finally
            {
                HandleDRDERR(IDirect3DSurface8.Release(pBackBufferSurface));
            }
            return res;
        }

        private BufferFormat GetBufferFormatFromSurface(IntPtr pSurface)
        {
            HandleDRDERR(IDirect3DSurface8.GetDesc(pSurface, out var desc));
            return new BufferFormat(desc);
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

        public void Screenshot(Action<IntPtr, BufferFormat> pixelDataHandler)
        {
            // Back buffer method
            HandleDRDERR(IDirect3DDevice8.GetBackBuffer(Device, 0, D3DBACKBUFFER_TYPE.TYPE_MONO, out var pBackBufferSurfaceValue));
            IntPtr pBackBufferSurface = new IntPtr(pBackBufferSurfaceValue);

            // Create rectangle from which data will be read (surface sized)
            BufferFormat format = GetBufferFormatFromSurface(pBackBufferSurface);
            RECT renderingRect = new RECT(0, 0, (int)format.width, (int)format.height);
            IntPtr rectPtr = Marshal.AllocHGlobal(Marshal.SizeOf(renderingRect));
            Marshal.StructureToPtr(renderingRect, rectPtr, false);
            try
            {
                // Lock pixels
                HandleDRDERR(IDirect3DSurface8.LockRect(pBackBufferSurface, out D3DLOCKED_RECT lockedRect, rectPtr, (ulong)LOCK_RECT_FLAGS.D3DLOCK_READONLY));
                pixelDataHandler(lockedRect.pBits, format);
            }
            finally
            {
                // Release resources
                HandleDRDERR(IDirect3DSurface8.UnlockRect(pBackBufferSurface));
                HandleDRDERR(IDirect3DSurface8.Release(pBackBufferSurface));
                Marshal.FreeHGlobal(rectPtr);
            }
        }

        #region Hooking
        private void RegisterHooks()
        {
            var moduleBase = _hookManager.GetModuleBase();
            // Window Procedure
            IntPtr windowProcPtr = new IntPtr(moduleBase.ToInt32() + MemoryAdresses.WINDOW_PROC_OFFSET);
            WindowProcedure = Marshal.GetDelegateForFunctionPointer<WindowProcedure_delegate>(windowProcPtr);
            _hookManager.RegisterHook(windowProcPtr, new WindowProcedure_delegate(WindowProcedure_Hook));

            // D3D8 functions
            var d3d8 = _hookManager.GetD3D8Base();

            // Create device
            IntPtr createDevicePtr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.CREATE_DEVICE_OFFSET);
            IDirect3D8.CreateDevice = Marshal.GetDelegateForFunctionPointer<CreateDevice_delegate>(createDevicePtr);
            _hookManager.RegisterHook(createDevicePtr, new CreateDevice_delegate(CreateDevice_Hook));

            // Reset device
            IntPtr resetDevicePtr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.RESET_DEVICE_OFFSET);
            IDirect3DDevice8.Reset = Marshal.GetDelegateForFunctionPointer<Reset_delegate>(resetDevicePtr);
            _hookManager.RegisterHook(resetDevicePtr, new Reset_delegate(Reset_Hook));

            // Present
            IntPtr presentPtr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.PRESENT_OFFSET);
            IDirect3DDevice8.Present = Marshal.GetDelegateForFunctionPointer<Present_delegate>(presentPtr);
            _hookManager.RegisterHook(presentPtr, new Present_delegate(Present_Hook));

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

            // Get image surface desc
            IntPtr getDescPtr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.GET_DESC_OFFSET);
            IDirect3DSurface8.GetDesc = Marshal.GetDelegateForFunctionPointer<GetDesc_delegate>(getDescPtr);

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
        void WindowProcedure_Hook(IntPtr hwnd, uint uMsg, uint wParam, uint lParam)
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

        private uint CreateDevice_Hook(IntPtr d3d8Interface, uint adapter, D3DDEVTYPE deviceType, IntPtr hFocusWindow, uint behaviorFlags, ref D3DPRESENT_PARAMETERS pPresentationParameters, ref IntPtr ppReturnedDeviceInterface)
        {
            UpdatePresentationParams(ref pPresentationParameters);
            return IDirect3D8.CreateDevice(d3d8Interface, adapter, deviceType, hFocusWindow, behaviorFlags, ref pPresentationParameters, ref ppReturnedDeviceInterface);
        }

        private uint Reset_Hook(IntPtr pDevice, ref D3DPRESENT_PARAMETERS pPresentationParameters)
        {
            UpdatePresentationParams(ref pPresentationParameters);
            return IDirect3DDevice8.Reset(pDevice, ref pPresentationParameters);
        }

        private void UpdatePresentationParams(ref D3DPRESENT_PARAMETERS pPresentationParameters)
        {
            const uint D3DPRESENTFLAG_LOCKABLE_BACKBUFFER = 0x00000001;
            // Make back buffer lockable for pixel capture
            pPresentationParameters.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
            // Update width/height
            pPresentationParameters.BackBufferWidth = this._renderParams.renderWidth;
            pPresentationParameters.BackBufferHeight = this._renderParams.renderHeight;
        }

        private uint Present_Hook(IntPtr pDevice, IntPtr pSourceRect, IntPtr pDestRec, uint hDestWindowOverride, IntPtr pDirtyRegion)
        {
            if (_renderParams.renderingEnabled)
            {
                return IDirect3DDevice8.Present(pDevice, pSourceRect, pDestRec, hDestWindowOverride, pDirtyRegion);
            } else
            {
                return 0; // Don't do anything, return OK
            }
        }
        #endregion

        #region window management functions
        static WindowProcedure_delegate WindowProcedure;
        #endregion
        #region D3D8 Functions
        static class IDirect3D8
        {
            public static CreateDevice_delegate CreateDevice;
        }

        static class IDirect3DDevice8
        {
            public static Reset_delegate Reset;
            public static GetDisplayMode_delegate GetDisplayMode;
            public static Present_delegate Present;
            public static CreateImageSurface_delegate CreateImageSurface;
            public static GetBackBuffer_delegate GetBackBuffer;
            public static CopyRects_delegate CopyRects;
        }

        static class IDirect3DSurface8
        {
            public static GetDesc_delegate GetDesc;
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
        delegate uint CreateDevice_delegate(IntPtr d3d8Interface, uint adapter, D3DDEVTYPE deviceType, IntPtr hFocusWindow, uint behaviorFlags, ref D3DPRESENT_PARAMETERS pPresentationParameters, ref IntPtr ppReturnedDeviceInterface);
        
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint Reset_delegate(IntPtr pDevice, ref D3DPRESENT_PARAMETERS pPresentationParameters);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint GetDisplayMode_delegate(IntPtr pDevice, ref D3DDISPLAYMODE pMode);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint Present_delegate(IntPtr pDevice, IntPtr pSourceRect, IntPtr pDestRec, uint hDestWindowOverride, IntPtr pDirtyRegion);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint CreateImageSurface_delegate(IntPtr pDevice, uint width, uint height, uint format, ref uint pSurface);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint GetBackBuffer_delegate(IntPtr pDevice, uint backBuffer, D3DBACKBUFFER_TYPE type, out uint pSurface);

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
        delegate uint GetDesc_delegate(IntPtr pSurface, out D3DSURFACE_DESC desc);

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
