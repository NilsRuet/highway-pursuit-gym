#pragma once
#include "../pch.h"

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
}
