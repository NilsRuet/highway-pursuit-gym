#pragma once
#include "../pch.h"

namespace Data
{
    typedef void IDirect3D8;
    typedef void IDirect3DDevice8;
    typedef void IDirect3DSurface8;
    typedef uint32_t D3DERR;
    constexpr uint32_t D3DERR_OK = 0;
    // Kind of an arbitrary choice to return a failing code if some D3D hook fails
    constexpr uint32_t D3DERR_NOTAVAILABLE = 2154;
    #pragma pack(push, 1)
    enum D3DBACKBUFFER_TYPE
    {
        MONO = 0,
        LEFT = 1,
        RIGHT = 2,
        FORCE_DWORD = 0x7fffffff
    };

    enum LOCK_RECT_FLAGS : uint64_t
    {
        D3DLOCK_READONLY = 0x00000010,
        D3DLOCK_DISCARD = 0x00002000,
        D3DLOCK_NOOVERWRITE = 0x00001000,
        D3DLOCK_NOSYSLOCK = 0x00000800,
        D3DLOCK_NO_DIRTY_UPDATE = 0x00008000
    };

    enum DISCL_FLAGS : uint32_t
    {
        DISCL_EXCLUSIVE = 0x00000001,
        DISCL_NONEXCLUSIVE = 0x00000002,
        DISCL_FOREGROUND = 0x00000004,
        DISCL_BACKGROUND = 0x00000008,
        DISCL_NOWINKEY = 0x00000010
    };

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
        D3DFMT_FORCE_DWORD = 0x7fffffff,
    };

    enum D3DDEVTYPE : uint32_t
    {
        D3DDEVTYPE_HAL = 1,
        D3DDEVTYPE_REF = 2,
        D3DDEVTYPE_SW = 3,
        D3DDEVTYPE_FORCE_DWORD = 0xffffffff
    };

    struct D3DLOCKED_RECT
    {
        int32_t Pitch;
        void* pBits;
    };

    struct D3DDISPLAYMODE
    {
        uint32_t Width;
        uint32_t Height;
        uint32_t RefreshRate;
        D3DFORMAT Format;
    };

    struct D3DPRESENT_PARAMETERS
    {
        uint32_t BackBufferWidth;
        uint32_t BackBufferHeight;
        D3DFORMAT BackBufferFormat;
        uint32_t BackBufferCount;
        uint32_t MultiSampleType;
        uint32_t SwapEffect;
        std::uintptr_t hDeviceWindow;
        uint32_t Windowed;
        uint32_t EnableAutoDepthStencil;
        D3DFORMAT AutoDepthStencilFormat;
        uint32_t Flags;

        uint32_t FullScreen_RefreshRateInHz;
        uint32_t FullScreen_PresentationInterval;
    };

    struct D3DSURFACE_DESC
    {
        D3DFORMAT Format;
        uint32_t Type;
        uint32_t Usage;
        uint32_t Pool;
        uint32_t Size;
        uint32_t MultiSampleType;
        uint32_t Width;
        uint32_t Height;
    };
    #pragma pack(pop)
}
