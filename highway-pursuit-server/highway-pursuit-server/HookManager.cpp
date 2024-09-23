#include "pch.h"
#include "HookManager.hpp"

HookManager::HookManager() : _unactivatedHooks()
{
    HookManager::Instance = this;
    MH_Initialize();

    // base module
    _moduleBase = (uintptr_t)GetModuleHandleA(nullptr);

    // Unused until lock/unlock are called
    _d3d8Lock = nullptr;
    _dInputLock = nullptr;
    _d3d8Interface = nullptr;
    _dInput8Interface = nullptr;
}

void HookManager::EnableAllNewHooks()
{
    // Enable the currently unactivated hooks
    for (LPVOID hook : _unactivatedHooks)
    {
        MH_EnableHook(hook);
    }
    _unactivatedHooks.clear();
}

void HookManager::Release()
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

uintptr_t HookManager::GetModuleBase() const
{
    return _moduleBase;
}

IDirect3D8* HookManager::GetD3D8Interface() const
{
    return this->_d3d8Interface;
}

IDirectInput8* HookManager::GetDInputInterface() const
{
    return this->_dInput8Interface;
}

void HookManager::HookAndLockD3D8Create()
{
    // Create semaphore
    _d3d8Lock = CreateSemaphore(nullptr, 0, 1, nullptr);

    HMODULE d3d8Module = GetModuleHandleA(_d3d8ModuleName.c_str());
    if (d3d8Module == NULL)
    {
        throw std::runtime_error("Couldn't find d3d8.dll.");
    }

    FARPROC createPtr = GetProcAddress(d3d8Module, "Direct3DCreate8");
    this->RegisterHook(createPtr, &Direct3DCreate8_StaticHook, &Direct3DCreate8_Base);
}


void HookManager::HookAndLockDirectInputCreate()
{
    // Create semaphore
    _dInputLock = CreateSemaphore(nullptr, 0, 1, nullptr);

    HMODULE dInputModule = GetModuleHandleA(_dinputModuleName.c_str());
    if (dInputModule == NULL)
    {
        throw std::runtime_error("Couldn't find DINPUT8.dll.");
    }

    FARPROC createPtr = GetProcAddress(dInputModule, "DirectInput8Create");
    this->RegisterHook(createPtr, &DirectInput8Create_StaticHook, &DirectInput8Create_Base);
}

void HookManager::UnlockD3D8Initialisation()
{
    ReleaseSemaphore(_d3d8Lock, 1, nullptr);
    CloseHandle(_d3d8Lock);
    _d3d8Lock = nullptr;
}

void HookManager::UnlockDInputInitialisation()
{
    ReleaseSemaphore(_dInputLock, 1, nullptr);
    CloseHandle(_dInputLock);
    _dInputLock = nullptr;
}

IDirect3D8* HookManager::Direct3DCreate8_Hook(UINT SDKVersion)
{
    IDirect3D8* res = Direct3DCreate8_Base(SDKVersion);
    _d3d8Interface = res;
    WaitForSingleObject(_d3d8Lock, INFINITE);
    return res;
}

HRESULT HookManager::DirectInput8Create_Hook(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, IDirectInput8** ppvOut, LPUNKNOWN punkOuter)
{
    HRESULT res = DirectInput8Create_Base(hinst, dwVersion, riidltf, ppvOut, punkOuter);
    _d3d8Interface = *ppvOut;
    WaitForSingleObject(_dInputLock, INFINITE);
    return res;
}


IDirect3D8* WINAPI HookManager::Direct3DCreate8_StaticHook(UINT SDKVersion)
{
    if (Instance != nullptr)
    {
        return Instance->Direct3DCreate8_Hook(SDKVersion);
    }
}

HRESULT WINAPI HookManager::DirectInput8Create_StaticHook(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, IDirectInput8** ppvOut, LPUNKNOWN punkOuter)
{
    if (Instance != nullptr)
    {
        return Instance->DirectInput8Create_Hook(hinst, dwVersion, riidltf, ppvOut, punkOuter);
    }
}


HookManager* HookManager::Instance = nullptr;
HookManager::Direct3DCreate8_t HookManager::Direct3DCreate8_Base = nullptr;
HookManager::DirectInput8Create_t HookManager::DirectInput8Create_Base = nullptr;