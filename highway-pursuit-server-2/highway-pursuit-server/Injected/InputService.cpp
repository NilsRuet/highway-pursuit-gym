#include "../pch.h"
#include "InputService.hpp"
#include "../HookManager.hpp"

using namespace Data;

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
    }

    // SetInput method
    void InputService::SetInput(const std::vector<Input>& inputs)
    {
    }

    // InputToKeyCode method
    int InputService::InputToKeyCode(Input input)
    {
    }

    // RegisterHooks method
    void InputService::RegisterHooks()
    {
    }
}