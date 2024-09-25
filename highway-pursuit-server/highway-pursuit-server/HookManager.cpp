#include "pch.h"
#include "HookManager.hpp"

HookManager::HookManager()
{
    MH_Initialize();

    // base module
    _moduleBase = (uintptr_t)GetModuleHandleA(nullptr);
    _d3d8Module = GetModuleHandleA(_d3d8ModuleName.c_str());
    if (_d3d8Module == 0)
    {
        throw std::runtime_error("d3d8.dll not found");
    }
    _directInput8Module = GetModuleHandleA(_dinputModuleName.c_str());
    if (_directInput8Module == 0)
    {
        throw std::runtime_error("DINPUT8.dll not found");
    }
}

void HookManager::EnableHooks()
{
    MH_EnableHook(MH_ALL_HOOKS);
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

HMODULE HookManager::GetD3D8() const
{
    return _d3d8Module;
}

HMODULE HookManager::GetDirectInput8() const
{
    return _directInput8Module;
}