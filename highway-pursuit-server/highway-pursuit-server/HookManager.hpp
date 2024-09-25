#pragma once
#include "MinHook.h"
#include "Data/ServerTypes.hpp"
#include "Data/D3D8.hpp"

using namespace Data;

class HookManager
{
public:
    HookManager();
    void EnableHooks();
    void Release();

    template <typename T>
    void RegisterHook(const LPVOID& proc, const LPVOID& hook, T* pOriginal)
    {
        MH_STATUS res = MH_CreateHook(proc, hook, reinterpret_cast<LPVOID*>(pOriginal));
        if (res != MH_OK)
        {
            throw Data::MinHookException(res);
        }
    }

    uintptr_t GetModuleBase() const;
    HMODULE GetD3D8() const;
    HMODULE GetDirectInput8() const;

private:
    const std::string _d3d8ModuleName = "d3d8.dll";
    const std::string _dinputModuleName = "DINPUT8.dll";

    uintptr_t _moduleBase;
    HMODULE _d3d8Module;
    HMODULE _directInput8Module;
};

