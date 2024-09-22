#include "../pch.h"
#include "EpisodeService.hpp"
#include "MemoryAddresses.hpp"

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
        Reset_Base();
    }

    // NewLife method
    void EpisodeService::NewLife()
    {
        Respawn_Base(0x1);
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
        EpisodeService::Instance = this;

        // Hooked functions
        uintptr_t base = _hookManager->GetModuleBase();
        LPVOID setLivesPtr = reinterpret_cast<LPVOID>(_hookManager->GetModuleBase() + MemoryAddresses::SET_LIVES_OFFSET);
        _hookManager->RegisterHook(setLivesPtr, &SetLives_StaticHook, &SetLives_Base);

        // Other functions
        Reset_Base = (Reset_t)(base + MemoryAddresses::RESET_OFFSET);
        Respawn_Base = (Respawn_t)(base + MemoryAddresses::RESPAWN_OFFSET);
    }

    void EpisodeService::SetLives_Hook(uint8_t value)
    {
        SetLives_Base(value);
        // If one life was lost
        if (value == Data::HighwayPursuitConstants::CHEATED_CONSTANT_LIVES - 1)
        {
            _terminated = true;
        }
    }

    void __cdecl EpisodeService::SetLives_StaticHook(uint8_t lives)
    {
        if (EpisodeService::Instance != nullptr)
        {
            EpisodeService::Instance->SetLives_Hook(lives);
        }
    }

    EpisodeService* EpisodeService::Instance = nullptr;
    EpisodeService::Reset_t EpisodeService::Reset_Base = nullptr;
    EpisodeService::Respawn_t EpisodeService::Respawn_Base = nullptr;
    EpisodeService::SetLives_t EpisodeService::SetLives_Base = nullptr;
}