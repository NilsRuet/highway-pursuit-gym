#include "../pch.h"
#include "UpdateService.hpp"
#include "../Injected/MemoryAddresses.hpp"

namespace Injected
{
    UpdateService::UpdateService(std::shared_ptr<HookManager> hookManager, bool isRealTime, HANDLE lockServerPool, HANDLE lockUpdatePool, float FPS, long performanceCounterFrequency) :
        _hookManager(hookManager),
        _isRealTime(isRealTime),
        _lockServerPool(lockServerPool),
        _lockUpdatePool(lockUpdatePool),
        _FPS(FPS),
        _performanceCounterFrequency(performanceCounterFrequency),
        _performanceCount(0),
        _useSemaphores(true)
    {
        _counterTicksPerFrame = static_cast<long>(std::ceil(_performanceCounterFrequency / _FPS));
        RegisterHooks();
    }

    void UpdateService::UpdateTime()
    {
        if (!_isRealTime)
        {
            _performanceCount += _counterTicksPerFrame;
        }
    }

    void UpdateService::DisableSemaphores()
    {
        _useSemaphores = false;
    }

    void UpdateService::RegisterHooks()
    {
        if (!_isRealTime)
        {
            // Kernel32 hmodule
          /*  HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
            if (kernel32 == NULL)
            {
                throw std::runtime_error("Couldn't find kernel32.dll.");
            }*/

            // Hook performance counter functions
          /*  FARPROC qpc = GetProcAddress(kernel32, "QueryPerformanceCounter");
            auto qpc_hook = [this](long* counter) { return this->QueryPerformanceCounter_Hook(counter); };
            _hookManager->RegisterHook(qpc, &qpc_hook, reinterpret_cast<LPVOID*>(&QueryPerformanceCounter_gateway));

            FARPROC qpf = GetProcAddress(kernel32, "QueryPerformanceFrequency");
            auto qpf_hook = [this](long* frequency) { return this->QueryPerformanceFrequency_Hook(frequency); };
            _hookManager->RegisterHook(qpc, &qpf_hook, reinterpret_cast<LPVOID*>(&QueryPerformanceFrequency_gateway));*/
        }

        // Update function
        HPLogger::LogDebug("Hi");
        //void* updatePtr = _hookManager->GetModuleBase() + Injected::MemoryAddresses::UPDATE_OFFSET;
        //auto update_hook = [this](void) { return this->Update_Hook(); };
        //_hookManager->RegisterHook(updatePtr, &update_hook, reinterpret_cast<LPVOID*>(&Update_gateway));
    }

    bool UpdateService::QueryPerformanceFrequency_Hook(long* lpFrequency) const
    {
        if (_isRealTime)
        {
            throw std::runtime_error("QueryPerformanceFrequency_Hook called in real time mode.");
        }
        *lpFrequency = _performanceCounterFrequency;
        return TRUE;
    }

    bool UpdateService::QueryPerformanceCounter_Hook(long* lpPerformanceCount)
    {
        if (_isRealTime)
        {
            throw std::runtime_error("QueryPerformanceCounter_Hook called in real time mode.");
        }
        if (_performanceCount == 0)
        {
            QueryPerformanceCounter_gateway(lpPerformanceCount);
        }
        *lpPerformanceCount = _performanceCount;
        return TRUE;
    }

    void UpdateService::Update_Hook()
    {
       /* if (_useSemaphores)
        {
            WaitForSingleObject(_lockUpdatePool, INFINITE);
        }*/
        this->UpdateTime();
        Update_gateway();
       /* if (_useSemaphores)
        {
            ReleaseSemaphore(_lockServerPool, 1, nullptr);
        }*/
    }
}
