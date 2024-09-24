#include "pch.h"
#include "HookManager.hpp"

HookManager::HookManager() : _unactivatedHooks()
{
    HookManager::Instance = this;
    MH_Initialize();

    // base module
    _moduleBase = (uintptr_t)GetModuleHandleA(nullptr);

    // Unused until lock/unlock are called
    _d3d8WaitForMainThread = nullptr;
    _d3d8WaitForHookThread = nullptr;
    _dInputWaitForMainThread = nullptr;
    _dInputWaitForHookThread = nullptr;

    _toCallWithD3D8 = nullptr;
    _toCallWithDirectInput8 = nullptr;
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

void HookManager::HookAndLockD3D8Create()
{
    // Create semaphore
    _d3d8WaitForMainThread = CreateSemaphore(nullptr, 0, 1, nullptr);
    _d3d8WaitForHookThread = CreateSemaphore(nullptr, 0, 1, nullptr);

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
    _dInputWaitForMainThread = CreateSemaphore(nullptr, 0, 1, nullptr);
    _dInputWaitForHookThread = CreateSemaphore(nullptr, 0, 1, nullptr);

    HMODULE dInputModule = GetModuleHandleA(_dinputModuleName.c_str());
    if (dInputModule == NULL)
    {
        throw std::runtime_error("Couldn't find DINPUT8.dll.");
    }

    FARPROC createPtr = GetProcAddress(dInputModule, "DirectInput8Create");
    this->RegisterHook(createPtr, &DirectInput8Create_StaticHook, &DirectInput8Create_Base);
}

IDirect3D8* HookManager::Direct3DCreate8_Hook(UINT SDKVersion)
{
    HPLogger::LogDebug("Direct3D create hook");
    IDirect3D8* d3d8 = Direct3DCreate8_Base(SDKVersion);
    HPLogger::LogDebug("Assign d3d interface");

    // Notify initialisation
    ReleaseSemaphore(_d3d8WaitForMainThread, 1, nullptr);
    // Wait for the rendering service, which needs to be initialized
    HPLogger::LogDebug("Wait service");
    WaitForSingleObject(_d3d8WaitForHookThread, INFINITE);
    HPLogger::LogDebug("init service");

    if (_toCallWithD3D8 != nullptr)
    {
        _toCallWithD3D8(d3d8);
    }

    // Notify rendering service initialized
    ReleaseSemaphore(_d3d8WaitForMainThread, 1, nullptr);
    // Wait for the hooks to be initialized
    HPLogger::LogDebug("Wait hooks");
    WaitForSingleObject(_d3d8WaitForHookThread, INFINITE);

    return d3d8;
}

HRESULT HookManager::DirectInput8Create_Hook(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, IDirectInput8** ppvOut, LPUNKNOWN punkOuter)
{
    HRESULT res = DirectInput8Create_Base(hinst, dwVersion, riidltf, ppvOut, punkOuter);
    // Notify initialisation
    ReleaseSemaphore(_dInputWaitForMainThread, 1, nullptr);
    // Wait for the input service, which needs to be initialized
    WaitForSingleObject(_dInputWaitForHookThread, INFINITE);

    if (_toCallWithDirectInput8 != nullptr)
    {
        _toCallWithDirectInput8(*ppvOut);
    }

    // Notify input service initialized
    ReleaseSemaphore(_dInputWaitForMainThread, 1, nullptr);
    // Wait for the hooks to be initialized
    WaitForSingleObject(_dInputWaitForHookThread, INFINITE);
    return res;
}

void HookManager::InitializeWithD3D8(std::function<void(IDirect3D8*)> initializer)
{
    // Wait for d3d8 initialization
    WaitForSingleObject(_d3d8WaitForMainThread, INFINITE);
    // Provide the rendering service initializer
    this->_toCallWithD3D8 = initializer;
    ReleaseSemaphore(_d3d8WaitForHookThread, 1, nullptr);

    // Wait for the call
    WaitForSingleObject(_d3d8WaitForMainThread, INFINITE);
    // Don't release the game yet, as hooks are yet to be initialized
    this->_toCallWithD3D8 = nullptr;
}

void HookManager::InitializeWithDirectInput(std::function<void(IDirectInput8*)> initializer)
{
    // Wait for direct input initialization
    WaitForSingleObject(_dInputWaitForMainThread, INFINITE);
    // Provide the input service initializer
    this->_toCallWithDirectInput8 = initializer;
    ReleaseSemaphore(_dInputWaitForHookThread, 1, nullptr);

    // Wait for the call
    WaitForSingleObject(_dInputWaitForMainThread, INFINITE);
    // Don't release the game yet, as hooks are yet to be initialized
    this->_toCallWithDirectInput8 = nullptr;
}

void HookManager::UnlockD3D8Initialisation()
{
    ReleaseSemaphore(_d3d8WaitForHookThread, 1, nullptr);
    CloseHandle(_d3d8WaitForHookThread);
    CloseHandle(_d3d8WaitForMainThread);
    _d3d8WaitForHookThread = nullptr;
    _d3d8WaitForMainThread = nullptr;
}

void HookManager::UnlockDInputInitialisation()
{
    ReleaseSemaphore(_dInputWaitForHookThread, 1, nullptr);
    CloseHandle(_dInputWaitForHookThread);
    CloseHandle(_dInputWaitForMainThread);
    _dInputWaitForHookThread = nullptr;
    _dInputWaitForMainThread = nullptr;
}

IDirect3D8* WINAPI HookManager::Direct3DCreate8_StaticHook(UINT SDKVersion)
{
    if (Instance != nullptr)
    {
        return Instance->Direct3DCreate8_Hook(SDKVersion);
    }
    return nullptr;
}

HRESULT WINAPI HookManager::DirectInput8Create_StaticHook(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, IDirectInput8** ppvOut, LPUNKNOWN punkOuter)
{
    if (Instance != nullptr)
    {
        return Instance->DirectInput8Create_Hook(hinst, dwVersion, riidltf, ppvOut, punkOuter);
    }
    return -1;
}


HookManager* HookManager::Instance = nullptr;
HookManager::Direct3DCreate8_t HookManager::Direct3DCreate8_Base = nullptr;
HookManager::DirectInput8Create_t HookManager::DirectInput8Create_Base = nullptr;