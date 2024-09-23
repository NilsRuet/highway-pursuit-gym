#pragma once
#include "MinHook.h"
#include "Data/ServerTypes.hpp"
#include "Data/D3D8.hpp"

using namespace Data;

class HookManager
{
public:
    HookManager();
    void EnableAllNewHooks();
    void Release();

    uintptr_t GetModuleBase() const;

    void HookAndLockD3D8Create();
    void HookAndLockDirectInputCreate();

    IDirect3D8* GetD3D8Interface() const;
    IDirectInput8* GetDInputInterface() const;

    void UnlockD3D8Initialisation();
    void UnlockDInputInitialisation();

    template <typename T>
    void RegisterHook(const LPVOID& proc, const LPVOID& hook, T* pOriginal)
    {
        MH_STATUS res = MH_CreateHook(proc, hook, reinterpret_cast<LPVOID*>(pOriginal));
        if (res != MH_OK)
        {
            throw Data::MinHookException(res);
        }
        else
        {
            _unactivatedHooks.push_back(proc);
        }
    }

private:
    static HookManager* Instance;

    const std::string _d3d8ModuleName = "d3d8.dll";
    const std::string _dinputModuleName = "DINPUT8.dll";

    HANDLE _d3d8Lock;
    HANDLE _dInputLock;
    IDirect3D8* _d3d8Interface;
    IDirectInput8* _dInput8Interface;

    std::vector<LPVOID> _unactivatedHooks;
    uintptr_t _moduleBase;

    // Hooks
    IDirect3D8* Direct3DCreate8_Hook(UINT SDKVersion);
    HRESULT DirectInput8Create_Hook(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, IDirectInput8** ppvOut, LPUNKNOWN punkOuter);

    // Function signatures
    typedef IDirect3D8* (WINAPI* Direct3DCreate8_t)(UINT);
    typedef HRESULT(WINAPI* DirectInput8Create_t)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, IDirectInput8** ppvOut, LPUNKNOWN punkOuter);

    // Static hooks
    static IDirect3D8* WINAPI Direct3DCreate8_StaticHook(UINT SDKVersion);
    static HRESULT WINAPI DirectInput8Create_StaticHook(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, IDirectInput8** ppvOut, LPUNKNOWN punkOuter);

    // Original functions
    static Direct3DCreate8_t Direct3DCreate8_Base;
    static DirectInput8Create_t DirectInput8Create_Base;
};

