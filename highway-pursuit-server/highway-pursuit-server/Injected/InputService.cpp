#include "../pch.h"
#include "InputService.hpp"
#include "MemoryAddresses.hpp"

namespace Injected
{
    InputService::InputService(std::shared_ptr<HookManager> hookManager)
        : _hookManager(hookManager)
    {
        FindAddresses();
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

    void InputService::FindAddresses()
    {
        // Get entry point
        DirectInputCreate_Base = reinterpret_cast<DirectInput8Create_t>(GetProcAddress(_hookManager->GetDirectInput8(), "DirectInput8Create"));
        if (DirectInputCreate_Base == nullptr)
        {
            throw std::runtime_error("Failed to get DirectInputCreate");
        }

        // Create direct input
        HMODULE hInstance = GetModuleHandle(NULL);
        DirectInput8Wrapper dinput(hInstance, HighwayPursuitConstants::DINPUT_VERSION, HighwayPursuitConstants::IID_IDirectInput8A);
        uintptr_t* dinput_vtable = *reinterpret_cast<uintptr_t**>(dinput.DirectInput());

        DirectInputRelease_Base = reinterpret_cast<DirectInput8Release_t>(dinput_vtable[MemoryAddresses::DINPUT_RELEASE_OFFSET]);
        DirectInputCreateDevice_Base = reinterpret_cast<DirectInput8CreateDevice_t>(dinput_vtable[MemoryAddresses::DINPUT_CREATE_DEVICE_OFFSET]);

        // Create direct input device
        DirectInputDevice8Wrapper device(dinput.DirectInput(), HighwayPursuitConstants::GUID_SysKeyboard);
        uintptr_t* device_vtable = *reinterpret_cast<uintptr_t**>(device.Device());

        DirectInputReleaseDevice_Base = reinterpret_cast<DirectInput8ReleaseDevice_t>(device_vtable[MemoryAddresses::DINPUT_RELEASE_DEVICE_OFFSET]);
        SetCooperativeLevel_Base = reinterpret_cast<SetCooperativeLevel_t>(device_vtable[MemoryAddresses::SET_COOPERATIVE_LEVEL_OFFSET]);
        GetDeviceState_Base = reinterpret_cast<GetDeviceState_t>(device_vtable[MemoryAddresses::GET_DEVICE_STATE_OFFSET]);
    }

    // RegisterHooks method
    void InputService::RegisterHooks()
    {
        InputService::Instance = this;

        _hookManager->RegisterHook(SetCooperativeLevel_Base, &SetCooperativeLevel_StaticHook, &SetCooperativeLevel_Base);
        _hookManager->RegisterHook(GetDeviceState_Base, &GetDeviceState_StaticHook, &GetDeviceState_Base);
    }

    void InputService::HandleD3DERR(D3DERR errorCode)
    {
        if (errorCode > 0)
        {
            throw D3D8Exception(errorCode);
        }
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


    InputService::DirectInput8Wrapper::DirectInput8Wrapper(HMODULE hinst, DWORD dwVersion, REFIID riidltf)
    {
        IDirectInput8* dinput;
        HandleD3DERR(DirectInputCreate_Base(hinst, dwVersion, riidltf, &dinput, nullptr));
        _dinput = dinput;
    }

    InputService::DirectInput8Wrapper::~DirectInput8Wrapper()
    {
        HandleD3DERR(DirectInputRelease_Base(_dinput));
    }

    IDirect3D8* InputService::DirectInput8Wrapper::DirectInput() const
    {
        return _dinput;
    }

    InputService::DirectInputDevice8Wrapper::DirectInputDevice8Wrapper(IDirectInput8* dinput, REFGUID rguid)
    {
        IDirectInputDevice8* pDevice;
        HandleD3DERR(DirectInputCreateDevice_Base(dinput, rguid, &pDevice, nullptr));
        _pDevice = pDevice;
    }

    InputService::DirectInputDevice8Wrapper::~DirectInputDevice8Wrapper()
    {
        HandleD3DERR(DirectInputReleaseDevice_Base(_pDevice));
    }

    IDirect3DSurface8* InputService::DirectInputDevice8Wrapper::Device() const
    {
        return _pDevice;
    }

    InputService* InputService::Instance = nullptr;
    InputService::DirectInput8Create_t InputService::DirectInputCreate_Base = nullptr;
    InputService::DirectInput8Release_t InputService::DirectInputRelease_Base = nullptr;
    InputService::DirectInput8CreateDevice_t InputService::DirectInputCreateDevice_Base = nullptr;
    InputService::DirectInput8ReleaseDevice_t InputService::DirectInputReleaseDevice_Base = nullptr;
    InputService::SetCooperativeLevel_t InputService::SetCooperativeLevel_Base = nullptr;
    InputService::GetDeviceState_t InputService::GetDeviceState_Base = nullptr;
}