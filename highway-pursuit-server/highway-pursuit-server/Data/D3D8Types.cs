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

        public bool IsSameAs(D3DDISPLAYMODE displayMode)
        {
            return Width == displayMode.Width &&
                   Height == displayMode.Height &&
                   RefreshRate == displayMode.RefreshRate &&
                   Format == displayMode.Format;
        }

        public uint GetChannelCount()
        {
            switch (Format)
            {
                case D3DFORMAT.D3DFMT_X8R8G8B8:
                    return 4;
                default:
                    throw new HighwayPursuitException(ErrorCode.UNSUPPORTED_BACKBUFFER_FORMAT);
            }
        }
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
}
