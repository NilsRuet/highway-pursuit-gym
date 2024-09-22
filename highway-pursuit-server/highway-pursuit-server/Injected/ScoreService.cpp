#include "../pch.h"
#include "ScoreService.hpp"
#include "MemoryAddresses.hpp"

namespace Injected
{
    ScoreService::ScoreService(std::shared_ptr<HookManager> hookManager)
        : _hookManager(hookManager)
    {
        RegisterHooks();
    }

    uint32_t ScoreService::PullReward()
    {
        uint32_t scoreDelta = _newScore - _lastScore;
        _lastScore = _newScore;
        return scoreDelta;
    }

    // RegisterHooks method
    void ScoreService::RegisterHooks()
    {
        ScoreService::Instance = this;

        LPVOID setScorePtr = reinterpret_cast<LPVOID>(_hookManager->GetModuleBase() + MemoryAddresses::SET_SCORE_OFFSET);
        _hookManager->RegisterHook(setScorePtr, &SetScore_StaticHook, &SetScore_Base);
    }

    void ScoreService::SetScore_Hook(uint32_t score)
    {
        if (score == 0)
        {
            // If SetScore is called with 0, a reset happended
            _lastScore = 0;
        }
        _newScore = score;
        SetScore_Base(score);
    }

    void __cdecl ScoreService::SetScore_StaticHook(uint32_t score)
    {
        if (ScoreService::Instance != nullptr)
        {
            return ScoreService::Instance->SetScore_Hook(score);
        }
    }

    ScoreService* ScoreService::Instance = nullptr;
    ScoreService::SetScore_t ScoreService::SetScore_Base = nullptr;
}
