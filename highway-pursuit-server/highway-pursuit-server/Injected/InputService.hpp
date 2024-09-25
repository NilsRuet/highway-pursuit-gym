#pragma once
#include "../HookManager.hpp"

using namespace Data;

namespace Injected
{
    class InputService
    {
    public:
        InputService(std::shared_ptr<HookManager> hookManager);
        int GetInputCount();
        void SetInput(const std::vector<Input>& inputs);

    private:
        static void HandleD3DERR(D3DERR errorCode);
        static const uint8_t ACTIVE_KEY = 0x80;
        static const uint8_t MANUAL_CONTROL_KEY = 0x2A; // holding left shift enables manual keyboard control

        std::shared_ptr<HookManager> _hookManager;
        std::vector<Input> _currentInputs;

        uint32_t InputToOffset(Input input);
        uint32_t InputToKeyCode(Input input);
        void FindAddresses();
        void RegisterHooks();

        // Hooks
        static InputService* Instance;
        D3DERR SetCooperativeLevel_Hook(IDirectInputDevice8* pInput, HWND hwnd, DISCL_FLAGS dwFlags);
        D3DERR GetDeviceState_Hook(IDirectInputDevice8* pInput, DWORD cbData, LPVOID lpvData);

        // Function signatures
        typedef D3DERR(WINAPI* DirectInput8Create_t)(HINSTANCE, DWORD, REFIID, IDirectInput8**, LPUNKNOWN);
        typedef D3DERR(__stdcall* DirectInput8Release_t)(IDirect3DSurface8*);

        typedef D3DERR(__stdcall* DirectInput8CreateDevice_t)(IDirectInput8*, REFGUID, IDirectInputDevice8**, LPUNKNOWN);
        typedef D3DERR(__stdcall* DirectInput8ReleaseDevice_t)(IDirect3DSurface8*);
        typedef D3DERR(__stdcall* SetCooperativeLevel_t)(IDirectInputDevice8*, HWND, DWORD);
        typedef D3DERR(__stdcall* GetDeviceState_t)(IDirectInputDevice8* , DWORD, LPVOID);

        // Static hooks (entry points)
        static D3DERR __stdcall SetCooperativeLevel_StaticHook(IDirectInputDevice8* pInput, HWND hwnd, DISCL_FLAGS dwFlags);
        static D3DERR __stdcall GetDeviceState_StaticHook(IDirectInputDevice8* pInput, DWORD cbData, LPVOID lpvData);

        // Original functions
        static DirectInput8Create_t DirectInputCreate_Base;
        static DirectInput8Release_t DirectInputRelease_Base;
        static DirectInput8CreateDevice_t DirectInputCreateDevice_Base;
        static DirectInput8ReleaseDevice_t DirectInputReleaseDevice_Base;
        static SetCooperativeLevel_t SetCooperativeLevel_Base;
        static GetDeviceState_t GetDeviceState_Base;

        class DirectInput8Wrapper
        {
            IDirectInput8* _dinput;
        public:
            DirectInput8Wrapper(HMODULE hinst, DWORD dwVersion, REFIID riidltf);
            ~DirectInput8Wrapper();
            IDirect3D8* DirectInput() const;
        };

        class DirectInputDevice8Wrapper
        {
            IDirectInputDevice8* _pDevice;
        public:
            DirectInputDevice8Wrapper(IDirectInput8*, REFGUID rguid);
            ~DirectInputDevice8Wrapper();
            IDirect3DSurface8* Device() const;
        };
    };
}
