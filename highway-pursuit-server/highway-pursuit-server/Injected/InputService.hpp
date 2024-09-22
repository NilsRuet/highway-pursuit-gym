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
        static const uint8_t ACTIVE_KEY = 0x80;
        static const uint8_t MANUAL_CONTROL_KEY = 0x2A; // holding left shift enables manual keyboard control

        std::shared_ptr<HookManager> _hookManager;
        std::vector<Input> _currentInputs;

        uint32_t InputToOffset(Input input);
        uint32_t InputToKeyCode(Input input);
        void RegisterHooks();

        // Hooks
        static InputService* Instance;
        D3DERR SetCooperativeLevel_Hook(IDirectInputDevice8* pInput, HWND hwnd, DISCL_FLAGS dwFlags);
        D3DERR GetDeviceState_Hook(IDirectInputDevice8* pInput, DWORD cbData, LPVOID lpvData);

        // Function signatures
        typedef D3DERR(__stdcall* SetCooperativeLevel_t)(IDirectInputDevice8*, HWND, DWORD);
        typedef D3DERR(__stdcall* GetDeviceState_t)(IDirectInputDevice8* , DWORD, LPVOID);

        // Static hooks (entry points)
        static D3DERR __stdcall SetCooperativeLevel_StaticHook(IDirectInputDevice8* pInput, HWND hwnd, DISCL_FLAGS dwFlags);
        static D3DERR __stdcall GetDeviceState_StaticHook(IDirectInputDevice8* pInput, DWORD cbData, LPVOID lpvData);

        // Original functions
        static SetCooperativeLevel_t SetCooperativeLevel_Base;
        static GetDeviceState_t GetDeviceState_Base;
    };
}
