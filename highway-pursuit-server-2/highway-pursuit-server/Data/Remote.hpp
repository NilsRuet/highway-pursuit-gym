#pragma once
#include "../pch.h"

namespace Data
{
    #pragma pack(push, 1)
    struct ReturnCode
    {
        uint8_t code;

        ReturnCode(uint32_t code)
        {
            this->code = static_cast<uint8_t>(code);
        }
    };

    struct ServerInfo
    {
        uint32_t obsHeight;
        uint32_t obsWidth;
        uint32_t obsChannels;
        uint32_t actionCount;

        ServerInfo(uint32_t obsHeight, uint32_t obsWidth, uint32_t obsChannels, uint32_t actionCount)
            : obsHeight(obsHeight), obsWidth(obsWidth), obsChannels(obsChannels), actionCount(actionCount)
        {
        }
    };

    enum class InstructionCode : uint32_t
    {
        RESET_NEW_LIFE = 1,
        RESET_NEW_GAME = 2,
        STEP = 3,
        CLOSE = 0xFF
    };

    struct Instruction
    {
        InstructionCode code;

        Instruction(InstructionCode code) : code(code) {}
    };

    struct Info
    {
        float tps;
        float memory;

        Info(float tps, float memory) : tps(tps), memory(memory) {}
    };

    struct Reward
    {
        float reward;

        Reward(float reward) : reward(reward) {}
    };

    struct Termination
    {
        uint8_t terminated;
        uint8_t truncated;

        Termination(bool terminated, bool truncated)
        {
            this->terminated = BoolToByte(terminated);
            this->truncated = BoolToByte(truncated);
        }

        bool IsDone() const
        {
            return terminated > 0 || truncated > 0;
        }

    private:
        static uint8_t BoolToByte(bool value)
        {
            return value ? static_cast<uint8_t>(0x1) : static_cast<uint8_t>(0x0);
        }
    };
    #pragma pack(pop)
}
