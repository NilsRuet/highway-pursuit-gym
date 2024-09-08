using HighwayPursuitServer.Server;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    class Direct3D8Service
    {
        private readonly IHookManager _hookManager;
        public IntPtr Device => GetDevice();
        private IntPtr pSurface = IntPtr.Zero;
        private uint _currentWidth = 0;
        private uint _currentHeight = 0;

        public Direct3D8Service(IHookManager hookManager)
        {
            this._hookManager = hookManager;
            this.RegisterFunctions();
        }

        ~Direct3D8Service()
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

        private void EnsureSurface()
        {
            // Get display mode
            D3DDISPLAYMODE displayMode = new D3DDISPLAYMODE();
            HandleDRDERR(IDirect3DDevice8.GetDisplayMode(Device, ref displayMode));

            // Init width/height
            if (_currentWidth == 0 && _currentHeight == 0)
            {
                _currentWidth = displayMode.Width;
                _currentHeight = displayMode.Height;
            }

            // Flag about creating a new surface
            bool createSurface = (pSurface == IntPtr.Zero);

            // Release the current surface, and set the flag to create a new one
            if (_currentWidth != displayMode.Width || _currentHeight != displayMode.Height)
            {
                HandleDRDERR(IDirect3DSurface8.Release(pSurface));
                createSurface = true;
            }

            // Create surface if necessary
            if (createSurface)
            {
                uint pSurfaceValue = 0;
                HandleDRDERR(IDirect3DDevice8.CreateImageSurface(Device, displayMode.Width, displayMode.Height, (uint)D3DFORMAT.D3DFMT_X8R8G8B8, ref pSurfaceValue));
                pSurface = new IntPtr(pSurfaceValue);
                _currentWidth = displayMode.Width;
                _currentHeight = displayMode.Height;
            }
        }

        public void Screenshot()
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

                // TODO: do stuff! including removing the X8 channel?

            } finally
            {
                // Release resources
                HandleDRDERR(IDirect3DSurface8.UnlockRect(pSurface));
                HandleDRDERR(IDirect3DSurface8.Release(pBackBufferSurface));
            }
        }

        #region Hooking
        private void RegisterFunctions()
        {
            var d3d8 = _hookManager.GetD3D8Base();

            // Get display mode
            IntPtr getDisplayModeptr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.GET_DISPLAY_MODE_OFFSET);
            IDirect3DDevice8.GetDisplayMode = Marshal.GetDelegateForFunctionPointer<GetDisplayMode_delegate>(getDisplayModeptr);

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

    #region enums & structs
    public class D3DERR : Exception
    {
        // Error is an hcode (32 bits uint) : (isError, faculty, code) with sizes 1,15,16.
        // Only the code is informative, so we mask with FFFF
        public D3DERR(uint errCode) : base($"D3DERR: code {errCode & 0xFFFF}") { }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct D3DLOCKED_RECT
    {
        public int Pitch;
        public IntPtr pBits;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct D3DDISPLAYMODE
    {
        public uint Width;
        public uint Height;
        public uint RefreshRate;
        public D3DFORMAT Format;
    };

    public enum D3DBACKBUFFER_TYPE
    {
        TYPE_MONO = 0,
        LEFT = 1,
        RIGHT = 2,
        FORCE_DWORD = 0x7fffffff
    }

    [Flags]
    public enum LOCK_RECT_FLAGS : ulong
    {
        D3DLOCK_READONLY = 0x00000010L,
        D3DLOCK_DISCARD = 0x00002000L,
        D3DLOCK_NOOVERWRITE = 0x00001000L,
        D3DLOCK_NOSYSLOCK = 0x00000800L,
        D3DLOCK_NO_DIRTY_UPDATE = 0x00008000L
    }

    public enum D3DFORMAT
    {
        D3DFMT_UNKNOWN = 0,

        D3DFMT_R8G8B8 = 20,
        D3DFMT_A8R8G8B8 = 21,
        D3DFMT_X8R8G8B8 = 22,
        D3DFMT_R5G6B5 = 23,
        D3DFMT_X1R5G5B5 = 24,
        D3DFMT_A1R5G5B5 = 25,
        D3DFMT_A4R4G4B4 = 26,
        D3DFMT_R3G3B2 = 27,
        D3DFMT_A8 = 28,
        D3DFMT_A8R3G3B2 = 29,
        D3DFMT_X4R4G4B4 = 30,

        D3DFMT_A8P8 = 40,
        D3DFMT_P8 = 41,

        D3DFMT_L8 = 50,
        D3DFMT_A8L8 = 51,
        D3DFMT_A4L4 = 52,

        D3DFMT_V8U8 = 60,
        D3DFMT_L6V5U5 = 61,
        D3DFMT_X8L8V8U8 = 62,
        D3DFMT_Q8W8V8U8 = 63,
        D3DFMT_V16U16 = 64,
        D3DFMT_W11V11U10 = 65,


        D3DFMT_D16_LOCKABLE = 70,
        D3DFMT_D32 = 71,
        D3DFMT_D15S1 = 73,
        D3DFMT_D24S8 = 75,
        D3DFMT_D16 = 80,
        D3DFMT_D24X8 = 77,
        D3DFMT_D24X4S4 = 79,


        D3DFMT_VERTEXDATA = 100,
        D3DFMT_INDEX16 = 101,
        D3DFMT_INDEX32 = 102,

        D3DFMT_FORCE_DWORD = 0x7fffffff
    }
    #endregion
}
