#pragma once
#include "../HookManager.hpp"

namespace Injected
{
    class UpdateService
    {
    public:
        // Static instance ptr
        static UpdateService* Instance;
        
        // Constructor
        UpdateService(std::shared_ptr<HookManager> hookManager, bool isRealTime, HANDLE lockServerPool, HANDLE lockUpdatePool, float FPS, LARGE_INTEGER performanceCounterFrequency);
        
        // Methods
        void UpdateTime();
        void DisableSemaphores();

    private:
        // Members
        std::shared_ptr<HookManager> _hookManager;
        float _FPS;
        bool _isRealTime;
        HANDLE _lockServerPool;
        HANDLE _lockUpdatePool;
        LARGE_INTEGER _performanceCounterFrequency;
        LARGE_INTEGER _performanceCounter;
        long _counterTicksPerFrame;
        bool _useSemaphores;

        // private methods
        void RegisterHooks();

        // Hooks
        BOOL QueryPerformanceFrequency_Hook(LARGE_INTEGER* lpFrequency);
        BOOL QueryPerformanceCounter_Hook(LARGE_INTEGER* lpPerformanceCount);
        void Update_Hook();

        // Function signatures
        typedef BOOL(WINAPI* QueryPerformanceFrequency_t)(LARGE_INTEGER*);
        typedef BOOL(WINAPI* QueryPerformanceCounter_t)(LARGE_INTEGER*);
        typedef void(__cdecl* Update_t)(void);

        // Static hooks (entry points)
        static BOOL WINAPI QueryPerformanceFrequency_StaticHook(LARGE_INTEGER* lpFrequency);
        static BOOL WINAPI QueryPerformanceCounter_StaticHook(LARGE_INTEGER* lpPerformanceCount);
        static void __cdecl Update_StaticHook();

        // Original functions
        static QueryPerformanceCounter_t QueryPerformanceCounter_Base;
        static QueryPerformanceFrequency_t QueryPerformanceFrequency_Base;
        static Update_t Update_Base;
    };
}