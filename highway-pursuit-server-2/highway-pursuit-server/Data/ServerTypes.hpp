#pragma once
#include "../pch.h"
#include "MinHook.h"
#include "D3D8.hpp"

namespace Data
{
    enum class ErrorCode : uint8_t
    {
        NOT_ACK = 0xFF,
        ACKNOWLEDGED = 0,
        NATIVE_ERROR = 1,
        CLIENT_TIMEOUT = 2,
        GAME_TIMEOUT = 3,
        UNSUPPORTED_BACKBUFFER_FORMAT = 4,
        UNKNOWN_ACTION = 5,
        ENVIRONMENT_NOT_RESET = 6,
    };

    class MinHookException : public std::runtime_error
    {
    public:
        const MH_STATUS code;

        MinHookException(MH_STATUS code)
            : std::runtime_error(FormatErrorMessage(code)), code(code) {}

    private:
        static std::string FormatErrorMessage(MH_STATUS code)
        {
            std::ostringstream oss;
            oss << "MinHook error: 0x" << std::setfill('0') << std::hex << static_cast<int>(code);
            return oss.str();
        }
    };

    class HighwayPursuitException : public std::runtime_error
    {
    public:
        const ErrorCode code;

        HighwayPursuitException(ErrorCode code)
            : std::runtime_error(FormatErrorMessage(code)), code(code) {}

    private:
        static std::string FormatErrorMessage(ErrorCode code)
        {
            std::ostringstream oss;
            oss << "Highway pursuit server error: 0x" << std::setfill('0') << std::hex << static_cast<int>(code);
            return oss.str();
        }
    };

    class HighwayPursuitConstants
    {
    public:
        static const uint8_t CHEATED_CONSTANT_LIVES = 3;
    };

    struct BufferFormat
    {
        uint32_t width;
        uint32_t height;
        uint32_t channels;

        BufferFormat() :
            width(0),
            height(0),
            channels(0)
        {

        }

        BufferFormat(const D3DSURFACE_DESC* surface) :
            width(surface->Width),
            height(surface->Height),
            channels(FormatToChannels(surface->Format))
        {
        }

        // Returns size in bytes
        uint32_t Size() const
        {
            return width * height * channels;
        }

        static uint32_t FormatToChannels(D3DFORMAT format)
        {
            switch (format)
            {
            case D3DFORMAT::D3DFMT_X8R8G8B8:
                return 4;
            default:
                throw new HighwayPursuitException(ErrorCode::UNSUPPORTED_BACKBUFFER_FORMAT);
            }
        }
    };

    enum class Input : uint32_t
    {
        Accelerate,
        Brake,
        SteerL,
        SteerR,
        Fire,
        Oil,
        Smoke,
        Missiles
    };

    class InputUtils
    {
    public:
        static Input IndexToInput(int index)
        {
            switch (index)
            {
            case 0: return Input::Accelerate;
            case 1: return Input::Brake;
            case 2: return Input::SteerL;
            case 3: return Input::SteerR;
            case 4: return Input::Fire;
            case 5: return Input::Oil;
            case 6: return Input::Smoke;
            case 7: return Input::Missiles;
            default: throw HighwayPursuitException(ErrorCode::UNKNOWN_ACTION);
            }
        }
    };


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


    struct ServerParams
    {
        struct RenderParams
        {
            const uint32_t renderWidth;
            const uint32_t renderHeight;
            const bool renderingEnabled;

            RenderParams(uint32_t renderWidth, uint32_t renderHeight, bool renderingEnabled)
                : renderWidth(renderWidth), renderHeight(renderHeight), renderingEnabled(renderingEnabled)
            {
            }
        };

        static constexpr const char* serverMutexId = "a";
        static constexpr const char* clientMutexId = "b";
        static constexpr const char* returnCodeMemoryId = "0";
        static constexpr const char* serverInfoMemoryId = "1";
        static constexpr const char* instructionMemoryId = "2";
        static constexpr const char* observationMemoryId = "3";
        static constexpr const char* infoMemoryId = "4";
        static constexpr const char* rewardMemoryId = "5";
        static constexpr const char* actionMemoryId = "6";
        static constexpr const char* terminationMemoryId = "7";

        const bool isRealTime;
        const int frameskip;
        const RenderParams renderParams;
        const std::string serverMutexName;
        const std::string clientMutexName;
        const std::string returnCodeMemoryName;
        const std::string serverInfoMemoryName;
        const std::string instructionMemoryName;
        const std::string observationMemoryName;
        const std::string infoMemoryName;
        const std::string rewardMemoryName;
        const std::string actionMemoryName;
        const std::string terminationMemoryName;

        ServerParams(bool isRealTime, int frameskip, const RenderParams& renderOptions, const std::string& sharedResourcesPrefix)
            : isRealTime(isRealTime),
            frameskip(frameskip),
            renderParams(renderOptions),
            serverMutexName(sharedResourcesPrefix + serverMutexId),
            clientMutexName(sharedResourcesPrefix + clientMutexId),
            returnCodeMemoryName(sharedResourcesPrefix + returnCodeMemoryId),
            serverInfoMemoryName(sharedResourcesPrefix + serverInfoMemoryId),
            instructionMemoryName(sharedResourcesPrefix + instructionMemoryId),
            observationMemoryName(sharedResourcesPrefix + observationMemoryId),
            infoMemoryName(sharedResourcesPrefix + infoMemoryId),
            rewardMemoryName(sharedResourcesPrefix + rewardMemoryId),
            actionMemoryName(sharedResourcesPrefix + actionMemoryId),
            terminationMemoryName(sharedResourcesPrefix + terminationMemoryId)
        {
        }
    };
}
