#pragma once
#include "../pch.h"
#include "MinHook.h"

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
            oss << "Highway pursuit server error: 0x" << static_cast<int>(code) << " " << static_cast<int>(code);
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
            oss << "Highway pursuit server error: 0x" << static_cast<int>(code) << " " << static_cast<int>(code);
            return oss.str();
        }
    };

    class HighwayPursuitConstants
    {
    public:
        static const uint8_t CHEATED_CONSTANT_LIVES = 3;
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
