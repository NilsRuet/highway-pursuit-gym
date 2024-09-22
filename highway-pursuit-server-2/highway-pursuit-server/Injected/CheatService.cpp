#include "../pch.h"
#include "CheatService.hpp"

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
    }
}
