#include "../pch.h"
#include "InputService.hpp"
#include "MemoryAddresses.hpp"

namespace Injected
{
    InputService::InputService(std::shared_ptr<HookManager> hookManager)
        : _hookManager(hookManager)
    {
        RegisterHooks();
    }

    int InputService::GetInputCount()
    {
        return HighwayPursuitConstants::ACTION_COUNT;
    }

    void InputService::SetInput(const std::vector<Input>& inputs)
    {
        _currentInputs = inputs;
    }

    uint32_t InputService::InputToOffset(Input input)
    {
        switch (input)
        {
        case Input::Accelerate:
            return MemoryAddresses::ACCELERATE_OFFSET;
        case Input::Brake:
            return MemoryAddresses::BRAKE_OFFSET;
        case Input::SteerL:
            return MemoryAddresses::STEER_L_OFFSET;
        case Input::SteerR:
            return MemoryAddresses::STEER_R_OFFSET;
        case Input::Fire:
            return MemoryAddresses::FIRE_OFFSET;
        case Input::Oil:
            return MemoryAddresses::OIL_OFFSET;
        case Input::Smoke:
            return MemoryAddresses::SMOKE_OFFSET;
        case Input::Missiles:
            return MemoryAddresses::MISSILES_OFFSET;
        default:
            throw HighwayPursuitException(ErrorCode::UNKNOWN_ACTION);
        }
    }

    // InputToKeyCode method
    uint32_t InputService::InputToKeyCode(Input input)
    {
        auto offset = InputToOffset(input);
        auto module = _hookManager->GetModuleBase();
        uint32_t* keyCode = reinterpret_cast<uint32_t*>(module + offset);
        return *keyCode;
    }

    // RegisterHooks method
    void InputService::RegisterHooks()
    {
        InputService::Instance = this;

       /* LPVOID setCooperativeLevelPtr = reinterpret_cast<LPVOID>(_hookManager->GetDINPUTBase() + MemoryAddresses::SET_COOPERATIVE_LEVEL_OFFSET);
        _hookManager->RegisterHook(setCooperativeLevelPtr, &SetCooperativeLevel_StaticHook, &SetCooperativeLevel_Base);

        LPVOID GetDeviceStatePtr = reinterpret_cast<LPVOID>(_hookManager->GetDINPUTBase() + MemoryAddresses::GET_DEVICE_STATE_OFFSET);
        _hookManager->RegisterHook(GetDeviceStatePtr, &GetDeviceState_StaticHook, &GetDeviceState_Base);*/
    }

    D3DERR InputService::SetCooperativeLevel_Hook(IDirectInputDevice8* pInput, HWND hwnd, DISCL_FLAGS dwFlags)
    {
        // Make it so the app doesn't require foreground or exclusive access, to avoid failing keyboard acquisition
        DISCL_FLAGS nonBlockingFlags = DISCL_FLAGS::DISCL_BACKGROUND | DISCL_FLAGS::DISCL_NONEXCLUSIVE;
        return SetCooperativeLevel_Base(pInput, hwnd, nonBlockingFlags);
    }

    D3DERR InputService::GetDeviceState_Hook(IDirectInputDevice8* pInput, DWORD cbData, LPVOID lpvData)
    {
        D3DERR res = GetDeviceState_Base(pInput, cbData, lpvData);
        uint32_t deviceSize = cbData;
        uint8_t* pDeviceState = reinterpret_cast<uint8_t*>(lpvData);

        // Don't do anything in manual control
        if (pDeviceState[MANUAL_CONTROL_KEY] == ACTIVE_KEY)
        {
            return res;
        }

        // Update the modifiedState based on the inputs
        for (Input input : _currentInputs)
        {
            int keycode = InputToKeyCode(input);
            pDeviceState[keycode] = ACTIVE_KEY;
        }
        return res;
    }

    D3DERR __stdcall InputService::SetCooperativeLevel_StaticHook(IDirectInputDevice8* pInput, HWND hwnd, DISCL_FLAGS dwFlags)
    {
        if (InputService::Instance != nullptr)
        {
            return InputService::Instance->SetCooperativeLevel_Hook(pInput, hwnd, dwFlags);
        }
        return D3DERR_NOTAVAILABLE;
    }
    
    D3DERR __stdcall InputService::GetDeviceState_StaticHook(IDirectInputDevice8* pInput, DWORD cbData, LPVOID lpvData)
    {
        if (InputService::Instance != nullptr)
        {
            return InputService::Instance->GetDeviceState_Hook(pInput, cbData, lpvData);
        }
        return D3DERR_NOTAVAILABLE;
    }

    InputService* InputService::Instance = nullptr;
    InputService::SetCooperativeLevel_t InputService::SetCooperativeLevel_Base = nullptr;
    InputService::GetDeviceState_t InputService::GetDeviceState_Base = nullptr;
}