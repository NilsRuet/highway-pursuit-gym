#pragma once
#include "../Data/ServerTypes.hpp"
#include "../HookManager.hpp"

namespace Injected
{
    using namespace Data;
    class WindowService
	{
    public:
        WindowService(std::shared_ptr<HookManager> hookManager);

    private:
        // Members
        std::shared_ptr<HookManager> _hookManager;
        // private methods
        void RegisterHooks();
        
        // Function signatures
        typedef void(__stdcall* WindowProcedure_t)(HWND, UINT, WPARAM, LPARAM);

        // Static hooks (entry points)
        static void __stdcall WindowProcedure_StaticHook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        // Original functions
        static WindowProcedure_t WindowProcedure_Base;
    };
}

