#pragma once
#include "../HookManager.hpp"

namespace Injected
{
	class CheatService
	{
        public:
            CheatService(std::shared_ptr<HookManager> hookManager);
        private:
            static CheatService* Instance;

            std::shared_ptr<HookManager> _hookManager;
            void RegisterHooks();

            // Hooks
            uint32_t GetLives_Hook();

            // Function signatures
            typedef uint32_t(__cdecl* GetLives_t)(void);

            // Static hooks (entry points)
            static uint32_t __cdecl GetLives_StaticHook();

            // Original functions
            static GetLives_t GetLive_Base;
	};
}
