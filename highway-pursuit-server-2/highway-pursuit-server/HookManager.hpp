#pragma once

class HookManager
{
public:
    HookManager();
    void RegisterHook(const LPVOID& proc, const LPVOID& hook, LPVOID* pGatewayOut);
    void EnableHooks();
    void Release();
    uintptr_t GetModuleBase() const;
    uintptr_t GetD3D8Base() const;
    uintptr_t GetDINPUTBase() const;

private:
    const std::string _d3d8ModuleName = "d3d8.dll";
    const std::string _dinputModuleName = "DINPUT8.dll";
    uintptr_t _moduleBase;
    uintptr_t _d3d8Base;
    uintptr_t _dinputBase;
};

