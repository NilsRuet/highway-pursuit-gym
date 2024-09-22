#pragma once
#include "../HookManager.hpp"

namespace Injected
{
	class CheatService
	{
        public:
            CheatService(std::shared_ptr<HookManager> hookManager);
        private:
            std::shared_ptr<HookManager> _hookManager;
            void RegisterHooks();
	};
}
