#pragma once
#include "../HookManager.hpp"

namespace Injected
{
    class UpdateService
    {
    public:
        UpdateService(std::shared_ptr<HookManager> hookManager, bool isRealTime, HANDLE lockServerPool, HANDLE lockUpdatePool, float FPS, long performanceCounterFrequency);
        void UpdateTime();
        void DisableSemaphores();

    private:
        // Methods
        void RegisterHooks();

        // Members
        std::shared_ptr<HookManager> _hookManager;
        float _FPS;
        long _performanceCounterFrequency;
        long _counterTicksPerFrame;
        long _performanceCount;
        HANDLE _lockServerPool;
        HANDLE _lockUpdatePool;
        bool _isRealTime;
        bool _useSemaphores;

        // Function signatures
        typedef BOOL(WINAPI* QueryPerformanceFrequency_t)(long*);
        typedef BOOL(WINAPI* QueryPerformanceCounter_t)(long*);
        typedef void(__cdecl* Update_t)(void);

        // Gateway for original functions
        QueryPerformanceFrequency_t QueryPerformanceFrequency_gateway = nullptr;
        QueryPerformanceCounter_t QueryPerformanceCounter_gateway = nullptr;
        Update_t Update_gateway = nullptr;

        // Hooks
        bool WINAPI QueryPerformanceFrequency_Hook(long* lpFrequency) const;
        bool WINAPI QueryPerformanceCounter_Hook(long* lpPerformanceCount);
        void __cdecl Update_Hook();
    };
}