#include "pch.h"
#include "HookManager.hpp"

HookManager::HookManager()
{
    MH_Initialize();

    _moduleBase = (uintptr_t)GetModuleHandleA(nullptr); // base module
    _d3d8Base = (uintptr_t)GetModuleHandleA(_d3d8ModuleName.c_str());
    _dinputBase = (uintptr_t)GetModuleHandleA(_dinputModuleName.c_str());

    if (!_d3d8Base)
    {
        throw std::runtime_error("Couldn't find d3d8.dll.");
    }
    if (!_dinputBase)
    {
        throw std::runtime_error("Couldn't find DINPUT8.dll.");
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

uintptr_t HookManager::GetD3D8Base() const
{
    return _d3d8Base;
}

uintptr_t HookManager::GetDINPUTBase() const
{
    return _dinputBase;
}