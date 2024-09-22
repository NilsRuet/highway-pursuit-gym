#pragma once
#include "../HookManager.hpp"

namespace Injected
{

    class ScoreService
    {
    public:
        ScoreService(std::shared_ptr<HookManager> hookManager);
        uint32_t PullReward();

    private:
        std::shared_ptr<HookManager> _hookManager;
        uint32_t _lastScore = 0;
        uint32_t _newScore = 0;

        void RegisterHooks();

        // Hooks
        static ScoreService* Instance;
        void SetScore_Hook(uint32_t score);

        // Function signatures
        typedef void(__cdecl* SetScore_t)(uint32_t);

        // Static hooks (entry points)
        static void __cdecl SetScore_StaticHook(uint32_t score);

        // Original functions
        static SetScore_t SetScore_Base;
    };
}

