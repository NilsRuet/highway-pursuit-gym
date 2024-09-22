#include "../pch.h"
#include "CheatService.hpp"
#include "MemoryAddresses.hpp"

namespace Injected
{
    // Constructor
    CheatService::CheatService(std::shared_ptr<HookManager> hookManager)
        : _hookManager(hookManager)
    {
        RegisterHooks();
    }

    // RegisterHooks method
    void CheatService::RegisterHooks()
    {
        CheatService::Instance = this;

        LPVOID getLivesPtr = reinterpret_cast<LPVOID>(_hookManager->GetModuleBase() + MemoryAddresses::GET_LIVES_OFFSET);
        _hookManager->RegisterHook(getLivesPtr, &GetLives_StaticHook, &GetLive_Base);
    }

    uint32_t CheatService::GetLives_Hook()
    {
        return Data::HighwayPursuitConstants::CHEATED_CONSTANT_LIVES;
    }

    uint32_t __cdecl CheatService::GetLives_StaticHook(void)
    {
        if (CheatService::Instance != nullptr)
        {
            return CheatService::Instance->GetLives_Hook();
        }
        return Data::HighwayPursuitConstants::CHEATED_CONSTANT_LIVES;
    }

    CheatService* CheatService::Instance = nullptr;
    CheatService::GetLives_t CheatService::GetLive_Base = nullptr;
}
