#pragma once
#include "MinHook.h"
#include "Data/ServerTypes.hpp"

class HookManager
{
public:
    HookManager();
    void EnableHooks();
    void Release();
    uintptr_t GetModuleBase() const;
    uintptr_t GetD3D8Base() const;
    uintptr_t GetDINPUTBase() const;

    template <typename T>
    void RegisterHook(const LPVOID& proc, const LPVOID& hook, T* pOriginal)
    {
        MH_STATUS res = MH_CreateHook(proc, hook, reinterpret_cast<LPVOID*>(pOriginal));
        if (res != MH_OK)
        {
            throw Data::MinHookException(res);
        }
    }

private:
    const std::string _d3d8ModuleName = "d3d8.dll";
    const std::string _dinputModuleName = "DINPUT8.dll";
    uintptr_t _moduleBase;
    uintptr_t _d3d8Base;
    uintptr_t _dinputBase;
};

