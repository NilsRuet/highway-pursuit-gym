#include "../pch.h"
#include "InputService.hpp"

namespace Injected
{
    // Constructor
    InputService::InputService(std::shared_ptr<HookManager> hookManager)
        : _hookManager(hookManager)
    {
        RegisterHooks();
    }

    // GetInputCount method
    int InputService::GetInputCount()
    {
        return 0;
    }

    // SetInput method
    void InputService::SetInput(const std::vector<Input>& inputs)
    {
    }

    // InputToKeyCode method
    int InputService::InputToKeyCode(Input input)
    {
        return 0;
    }

    // RegisterHooks method
    void InputService::RegisterHooks()
    {
    }
}