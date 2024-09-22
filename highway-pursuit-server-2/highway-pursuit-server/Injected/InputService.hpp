#pragma once

namespace Injected
{
    class InputService
    {
    public:
        InputService(std::shared_ptr<HookManager> hookManager);

        int GetInputCount();
        void SetInput(const std::vector<Input>& inputs);

    private:
        std::shared_ptr<HookManager> _hookManager;
        std::unordered_map<Input, uint32_t> _inputToOffset;
        std::vector<Input> _currentInputs;

        void RegisterHooks();
        int InputToKeyCode(Input input);

        static constexpr uint8_t ACTIVE_KEY = 0x80;
#ifdef DEBUG
        static constexpr uint8_t MANUAL_CONTROL_KEY = 0x2A;
#endif
    };
}
