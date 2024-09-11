using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Exceptions
{
    public enum ErrorCode : byte
    {
        NOT_ACK = 0xFF,
        ACKNOWLEDGED = 0,
        NATIVE_ERROR = 1,
        CLIENT_TIMEOUT = 2,
        GAME_TIMEOUT = 3,
        UNSUPPORTED_BACKBUFFER_FORMAT = 4,
        UNKNOWN_ACTION = 5,
        ENVIRONMENT_NOT_RESET = 6,
    }

    class HighwayPursuitException : Exception
    {
        public readonly ErrorCode code;
        public HighwayPursuitException(ErrorCode code) : base($"Highway pursuit server error : 0x{(int)code:x} {code}")
        {
            this.code = code;
        }
    }
}
