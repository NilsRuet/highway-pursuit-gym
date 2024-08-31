using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    class Direct3D8Service
    {
        private readonly IHookManager _hookManager;
        public Direct3D8Service(IHookManager hookManager)
        {
            this._hookManager = hookManager;
            this.RegisterFunctions();
        }

        public void Screenshot()
        {
            uint pSurface = 0;
            CreateImageSurface(GetDevice(), 10u, 10u, (uint)D3DFORMAT.D3DFMT_A8R8G8B8, ref pSurface);
            //device->GetFrontBuffer(pSurface);
            //D3DLOCKED_RECT lockedRect;
            //pSurface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY);
            //pSurface->UnlockRect();
            //pSurface->Release();
        }

        #region Hooking
        private void RegisterFunctions()
        {
            var d3d8 = _hookManager.GetD3D8Base();

            // Create image surface
            IntPtr createImageSurfacePtr = new IntPtr(d3d8.ToInt32() + MemoryAdresses.CREATE_SURFACE_IMAGE_OFFSET);
            CreateImageSurface = Marshal.GetDelegateForFunctionPointer<CreateImageSurface_delegate>(createImageSurfacePtr);

            // TODO: 
            //get render width/height
            //device->GetFrontBuffer(pSurface);
            //D3DLOCKED_RECT lockedRect;
            //pSurface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY);
            //pSurface->UnlockRect();
            //pSurface->Release();
        }

        private IntPtr GetDevice()
        {
            return new IntPtr(Marshal.ReadInt32(new IntPtr(_hookManager.GetModuleBase().ToInt32() + MemoryAdresses.DEVICE_PTR_OFFSET)));
        }

        #region D3D8 Functions
        static CreateImageSurface_delegate CreateImageSurface;
        #endregion

        #region delegates        
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U8)]
        delegate long CreateImageSurface_delegate(IntPtr pDevice, uint width, uint height, uint format, ref uint pSurface);
        #endregion

        #endregion
    }

    enum D3DFORMAT
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
}
