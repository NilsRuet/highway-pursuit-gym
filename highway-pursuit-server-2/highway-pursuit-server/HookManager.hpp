#pragma once

class HookManager
{
public:
    HookManager();
    void RegisterHook(const LPVOID& proc, const LPVOID& hook, LPVOID* pGatewayOut);
    void EnableHooks();
    void Release();
    HMODULE GetModuleBase() const;
    HMODULE GetD3D8Base() const;
    HMODULE GetDINPUTBase() const;

private:
    const std::string _d3d8ModuleName = "d3d8.dll";
    const std::string _dinputModuleName = "DINPUT8.dll";
    HMODULE _moduleBase;
    HMODULE _d3d8Base;
    HMODULE _dinputBase;
};

