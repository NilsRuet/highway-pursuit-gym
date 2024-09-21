#include "../pch.h"
#include "UpdateService.hpp"
#include "../Injected/MemoryAddresses.hpp"

namespace Injected
{
    UpdateService* UpdateService::Instance = nullptr;

    static BOOL WINAPI QueryPerformanceFrequency_StaticHook(LARGE_INTEGER* lpPerformanceCount)
    {
        if (UpdateService::Instance != nullptr)
        {
            return UpdateService::Instance->QueryPerformanceFrequency_Hook(lpPerformanceCount);
        }
        return FALSE;
    }

    static BOOL WINAPI QueryPerformanceCounter_StaticHook(LARGE_INTEGER* lpPerformanceCount)
    {
        if (UpdateService::Instance != nullptr)
        {
            return UpdateService::Instance->QueryPerformanceCounter_Hook(lpPerformanceCount);
        }
        return FALSE;
    }

    static void __cdecl Update_StaticHook(void)
    {
        if (UpdateService::Instance != nullptr)
        {
            UpdateService::Instance->Update_Hook();
        }
    }

    UpdateService::UpdateService(std::shared_ptr<HookManager> hookManager, bool isRealTime, HANDLE lockServerPool, HANDLE lockUpdatePool, float FPS, LARGE_INTEGER performanceCounterFrequency) :
        _hookManager(hookManager),
        _isRealTime(isRealTime),
        _lockServerPool(lockServerPool),
        _lockUpdatePool(lockUpdatePool),
        _FPS(FPS),
        _performanceCounterFrequency(performanceCounterFrequency),
        _performanceCounter(),
        _useSemaphores(true)
    {
        _counterTicksPerFrame = static_cast<long>(std::ceil(_performanceCounterFrequency.QuadPart / _FPS));
        _performanceCounter.QuadPart = 0;
        RegisterHooks();
    }

    void UpdateService::UpdateTime()
    {
        if (!_isRealTime)
        {
            _performanceCounter.QuadPart += _counterTicksPerFrame;
        }
    }

    void UpdateService::DisableSemaphores()
    {
        _useSemaphores = false;
    }

    bool UpdateService::QueryPerformanceFrequency_Hook(LARGE_INTEGER* lpFrequency) const
    {
        if (_isRealTime)
        {
            throw std::runtime_error("QueryPerformanceFrequency_Hook called in real time mode.");
        }
        *lpFrequency = _performanceCounterFrequency;
        return TRUE;
    }

    bool UpdateService::QueryPerformanceCounter_Hook(LARGE_INTEGER* lpPerformanceCount)
    {
        bool res = FALSE;
        if (_isRealTime)
        {
            throw std::runtime_error("QueryPerformanceCounter_Hook called in real time mode.");
        }
        if (_performanceCounter.QuadPart == 0)
        {
            res = QueryPerformanceCounter_Base(&_performanceCounter);
        }
        *lpPerformanceCount = _performanceCounter;
        return res;
    }

    void UpdateService::Update_Hook()
    {
        /* if (_useSemaphores)
         {
             WaitForSingleObject(_lockUpdatePool, INFINITE);
         }*/
        this->UpdateTime();
        Update_Base();
        /* if (_useSemaphores)
         {
             ReleaseSemaphore(_lockServerPool, 1, nullptr);
         }*/
    }

    void UpdateService::RegisterHooks()
    {
        if (!_isRealTime)
        {
            // Kernel32 hmodule
            HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
            if (kernel32 == NULL)
            {
                throw std::runtime_error("Couldn't find kernel32.dll.");
            }

            // Hook performance counter functions
            FARPROC qpfPtr = GetProcAddress(kernel32, "QueryPerformanceFrequency");
            _hookManager->RegisterHook(qpfPtr, reinterpret_cast<LPVOID*>(&QueryPerformanceFrequency_StaticHook), reinterpret_cast<LPVOID*>(&QueryPerformanceFrequency_Base));

            // Hook performance counter functions
            FARPROC qpcPtr = GetProcAddress(kernel32, "QueryPerformanceCounter");
            _hookManager->RegisterHook(qpcPtr, reinterpret_cast<LPVOID*>(&QueryPerformanceCounter_StaticHook), reinterpret_cast<LPVOID*>(&QueryPerformanceCounter_Base));
        }

        UpdateService::Instance = this;
        // Update function
        LPVOID updatePtr = (LPVOID)(_hookManager->GetModuleBase() + Injected::MemoryAddresses::UPDATE_OFFSET);
        _hookManager->RegisterHook(updatePtr, reinterpret_cast<LPVOID*>(&Update_StaticHook), reinterpret_cast<LPVOID*>(&Update_Base));
    }

}
