#pragma once
#include "../HookManager.hpp"

namespace Injected
{
    class EpisodeService
    {
    public:
        EpisodeService(std::shared_ptr<HookManager> hookManager);

        void NewGame();
        void NewLife();
        bool PullTerminated();

    private:
        std::shared_ptr<HookManager> _hookManager;
        bool _terminated;

        void RegisterHooks();
    };

}
