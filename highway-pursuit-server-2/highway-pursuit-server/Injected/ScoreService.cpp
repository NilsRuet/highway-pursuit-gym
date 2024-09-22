#include "../pch.h"
#include "ScoreService.hpp"

namespace Injected
{
    // Constructor
    ScoreService::ScoreService(std::shared_ptr<HookManager> hookManager)
        : _hookManager(hookManager)
    {
        RegisterHooks();
    }

    // PullReward method
    int ScoreService::PullReward()
    {
        // Empty body
        return 0;
    }

    // ResetScore method
    void ScoreService::ResetScore()
    {
        // Empty body
    }

    // RegisterHooks method
    void ScoreService::RegisterHooks()
    {
        // Empty body
    }
}
