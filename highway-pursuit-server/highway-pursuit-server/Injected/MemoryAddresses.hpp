#pragma once
#include "../pch.h"

namespace Injected
{
    struct MemoryAddresses
    {
        // Highway pursuit function offsets
        static const uint32_t WINDOW_PROC_OFFSET = 0x166f0;
        static const uint32_t RESET_OFFSET = 0x22A70;
        static const uint32_t RESPAWN_OFFSET = 0x11D80;
        static const uint32_t UPDATE_OFFSET = 0x29420;
        static const uint32_t GET_LIVES_OFFSET = 0x15C70;
        static const uint32_t SET_LIVES_OFFSET = 0x15c50;
        static const uint32_t SET_SCORE_OFFSET = 0x15C00;
        static const uint32_t DEVICE_PTR_OFFSET = 0x960CC;
        // Highway pursuit data offsets
        static const uint32_t MAIN_WINDOW_HANDLE = 0x960B8;
        static const uint32_t FULLSCREEN_FLAG_OFFSET = 0x7C1D9;
        static const uint32_t CAMERA_ZOOM_ANIM_OFFSET = 0x7C1E4;
        static const uint32_t ACCELERATE_OFFSET = 0x969E8;
        static const uint32_t BRAKE_OFFSET = 0x969EC;
        static const uint32_t STEER_L_OFFSET = 0x969F0;
        static const uint32_t STEER_R_OFFSET = 0x969F4;
        static const uint32_t FIRE_OFFSET = 0x969F8;
        static const uint32_t OIL_OFFSET = 0x969FC;
        static const uint32_t SMOKE_OFFSET = 0x97A00;
        static const uint32_t MISSILES_OFFSET = 0x97A04;

        // IDirect3D8
        static const uint32_t GET_ADAPATER_DISPLAY_MODE_OFFSET = 0x8;
        static const uint32_t CREATE_DEVICE_OFFSET = 0xF;

        // IDirect3DDevice8
        static const uint32_t RESET_DEVICE_OFFSET = 0xE;
        static const uint32_t PRESENT_OFFSET = 0xF;
        static const uint32_t GET_BACK_BUFFER_OFFSET = 0x10;
        static const uint32_t DEVICE_RELEASE_OFFSET = 0x2;

        // IDirect3DSurface8
        static const uint32_t GET_DESC_OFFSET = 0x8;
        static const uint32_t LOCK_RECT_OFFSET = 0x9;
        static const uint32_t UNLOCK_RECT_OFFSET = 0xA;
        static const uint32_t SURFACE_RELEASE_OFFSET = 0x2;

        // DINPUT offsets
        static const uint32_t GET_DEVICE_STATE_OFFSET = 0xD550;
        static const uint32_t SET_COOPERATIVE_LEVEL_OFFSET = 0xC550;
    };
}
