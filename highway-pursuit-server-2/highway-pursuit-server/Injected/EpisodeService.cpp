#include "../pch.h"
#include "EpisodeService.hpp"
namespace Injected
{

    // Constructor
    EpisodeService::EpisodeService(std::shared_ptr<HookManager> hookManager)
        : _hookManager(hookManager),
        _terminated(false)
    {
        RegisterHooks();
    }

    // NewGame method
    void EpisodeService::NewGame()
    {
        // Reset();
    }

    // NewLife method
    void EpisodeService::NewLife()
    {
        // Respawn(0x1);
    }

    // PullTerminated method
    bool EpisodeService::PullTerminated()
    {
        bool res = _terminated;
        _terminated = false;
        return res;
    }

    // RegisterHooks method
    void EpisodeService::RegisterHooks()
    {
        
    }
}