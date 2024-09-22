#pragma once
#include "../HookManager.hpp"

namespace Injected
{

    class ScoreService
    {
    public:
        ScoreService(std::shared_ptr<HookManager> hookManager);
        int PullReward();
        void ResetScore();

    private:
        std::shared_ptr<HookManager> _hookManager;
        int _lastScore = 0;
        int _newScore = 0;

        void RegisterHooks();
    };
}

