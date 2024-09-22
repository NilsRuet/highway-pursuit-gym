#pragma once
#include <string>
#include <windows.h>

namespace HighwayPursuitLauncher
{
    // Constants to match argument positions
    const int ARG_PATH = 0;
    const int ARG_EXE = 1;
    const int ARG_DLL = 2;
    const int ARG_REAL_TIME = 3;
    const int ARG_FRAME_SKIP = 4;
    const int ARG_RESOLUTION = 5;
    const int ARG_ENABLE_RENDERING = 6;
    const int ARG_LOG_DIR_PATH = 7;
    const int ARG_SHARED_RESOURCES_PREFIX = 8;
    const int TOTAL_ARGS = 9;

    // Exit codes as enum
    enum ExitCode : int
    {
        Success = 0,
        InvalidArgs = 1,
        InjectionFailed = 2,
        UnknownError = 3,
    };

    // Function declarations
    static int launch(int argc, char* argv[]);
    static bool processArgs(int argc, char* argv[], std::string& targetExe, std::string& targetDll);
    static bool parseBool(std::string str);
    static bool tryParseResolution(const std::string& arg, unsigned int& width, unsigned int& height);
}
