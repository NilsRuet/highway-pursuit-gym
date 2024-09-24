#include "../pch.h"
#include "WindowService.hpp"
#include "MemoryAddresses.hpp"

namespace Injected
{
    WindowService::WindowService(std::shared_ptr<HookManager> hookManager) : _hookManager(hookManager)
    {
        RegisterHooks();
    }

    void WindowService::RegisterHooks()
    {
        LPVOID windowProcPtr = reinterpret_cast<LPVOID>(_hookManager->GetModuleBase() + MemoryAddresses::WINDOW_PROC_OFFSET);
        _hookManager->RegisterHook(windowProcPtr, &WindowProcedure_StaticHook, &WindowProcedure_Base);
    }

    // Hook entry points
    void __stdcall WindowService::WindowProcedure_StaticHook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        const WPARAM focused = 1;
        // Disable the "lose focus" callback to disable pausing
        if (uMsg == WM_ACTIVATEAPP || uMsg == WM_ACTIVATE)
        {
            WindowProcedure_Base(hwnd, uMsg, focused, lParam);
        }
        else
        {
            WindowProcedure_Base(hwnd, uMsg, wParam, lParam);
        }
    }

    WindowService::WindowProcedure_t WindowService::WindowProcedure_Base = nullptr;
}