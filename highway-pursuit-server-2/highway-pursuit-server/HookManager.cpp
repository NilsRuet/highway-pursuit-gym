#include "pch.h"
#include "HookManager.hpp"
#include "Data/ServerTypes.hpp"
#include "MinHook.h"

HookManager::HookManager()
{
    MH_Initialize();

    _moduleBase = GetModuleHandleA(nullptr); // base module
    _d3d8Base = GetModuleHandleA(_d3d8ModuleName.c_str());
    _dinputBase = GetModuleHandleA(_dinputModuleName.c_str());

    if (!_d3d8Base)
    {
        throw std::runtime_error("Couldn't find d3d8.dll.");
    }
    if (!_dinputBase)
    {
        throw std::runtime_error("Couldn't find DINPUT8.dll.");
    }
}

void HookManager::RegisterHook(const LPVOID& proc, const LPVOID& hook, LPVOID* pGatewayOut)
{
    MH_STATUS res = MH_CreateHook(proc, hook, pGatewayOut);
    if (res != MH_OK)
    {
        throw Data::MinHookException(res);
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

HMODULE HookManager::GetModuleBase() const
{
    return _moduleBase;
}

HMODULE HookManager::GetD3D8Base() const
{
    return _d3d8Base;
}

HMODULE HookManager::GetDINPUTBase() const
{
    return _dinputBase;
}