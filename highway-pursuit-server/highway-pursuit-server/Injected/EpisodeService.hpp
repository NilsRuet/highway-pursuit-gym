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

        // Hooks
        static EpisodeService* Instance;
        void SetLives_Hook(uint8_t value);

        // Function signatures
        typedef void(__cdecl* Reset_t)(void);
        typedef void(__cdecl* Respawn_t)(uint8_t);
        typedef void(__cdecl* SetLives_t)(uint8_t);

        // Static hooks (entry points)
        static void __cdecl SetLives_StaticHook(uint8_t value);

        // Original functions
        static Reset_t Reset_Base;
        static Respawn_t Respawn_Base;
        static SetLives_t SetLives_Base;
    };

}
