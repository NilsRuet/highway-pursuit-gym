using HighwayPursuitServer.Exceptions;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Data
{
    public class D3DERR : Exception
    {
        // Error is an hcode (32 bits uint) : (isError, faculty, code) with sizes 1,15,16.
        // Only the code is informative, so we mask with FFFF
        public D3DERR(uint errCode) : base($"D3DERR: code {errCode & 0xFFFF}") { }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT
    {
        public int left;
        public int top;
        public int right;
        public int bottom;

        public RECT(int left, int top, int right, int bottom)
        {
            this.left = left;
            this.top = top;
            this.right = right;
            this.bottom = bottom;
        }
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
    }

    [StructLayout(LayoutKind.Sequential)]
    struct D3DPRESENT_PARAMETERS
    {
        public uint BackBufferWidth;
        public uint BackBufferHeight;
        public D3DFORMAT BackBufferFormat;
        public uint BackBufferCount;
        public uint MultiSampleType; // enum
        public uint SwapEffect ;// enum
        public IntPtr hDeviceWindow;
        public uint Windowed; // bool
        public uint EnableAutoDepthStencil; // bool
        public D3DFORMAT AutoDepthStencilFormat;
        public uint Flags;

        public uint FullScreen_RefreshRateInHz;
        public uint FullScreen_PresentationInterval;

        public override string ToString()
        {
            return $"({BackBufferWidth}x{BackBufferHeight} {BackBufferFormat} [{BackBufferCount}], windowed:{Windowed}, flags:{Flags})";
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct D3DSURFACE_DESC
    {
        public D3DFORMAT Format;
        public uint Type;// enum
        public uint Usage;
        public uint Pool;// enum
        public uint Size;
        public uint MultiSampleType;// enum
        public uint Width;
        public uint Height;
    }

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

        D3DFMT_FORCE_DWORD = 0x7fffffff,
    }

    public enum D3DDEVTYPE : uint
    {
        D3DDEVTYPE_HAL = 1,
        D3DDEVTYPE_REF = 2,
        D3DDEVTYPE_SW = 3,

        D3DDEVTYPE_FORCE_DWORD = 0xffffffff
    }
}
