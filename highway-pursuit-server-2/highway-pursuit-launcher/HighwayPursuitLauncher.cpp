#include "HighwayPursuitLauncher.hpp"
#include "Injection.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>


int main(int argc, char* argv[])
{
    HighwayPursuitLauncher::launch(argc, argv);
}

namespace HighwayPursuitLauncher
{
    static int launch(int argc, char* argv[])
    {
        std::string targetExe, targetDll;

        // Argument processing
        if (!processArgs(argc, argv, targetExe, targetDll))
        {
            std::cerr << "Invalid arguments" << std::endl;
            return ExitCode::InvalidArgs;
        }

        try
        {
            // Real-time
            bool isRealTime = parseBool(argv[ARG_REAL_TIME]);

            // Frameskip
            int frameSkip = std::stoi(argv[ARG_FRAME_SKIP]);
            {
                if (frameSkip < 1) frameSkip = 1;
            }

            // Resolution
            unsigned int renderWidth, renderHeight;
            if (!tryParseResolution(argv[ARG_RESOLUTION], renderWidth, renderHeight))
            {
                renderWidth = 640;
                renderHeight = 480;
            }

            // Rendering enabled
            bool renderEnabled = parseBool(argv[ARG_ENABLE_RENDERING]);
            

            // Inject the DLL into the target process
            auto args = Shared::HighwayPursuitArgs(isRealTime, frameSkip, renderWidth, renderHeight, renderEnabled, argv[ARG_LOG_DIR_PATH], argv[ARG_SHARED_RESOURCES_PREFIX]);
            if (!Injection::CreateAndInject(targetExe, targetDll, args))
            {
                return ExitCode::InjectionFailed;
            }

        }
        catch (const std::exception& e)
        {
            std::cerr << "Injection failed: " << e.what() << std::endl;
            return ExitCode::UnknownError;
        }

        return ExitCode::Success;
    }

    static bool parseBool(std::string str)
    {
        bool res;
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
        std::istringstream(str) >> std::boolalpha >> res;
        return res;
    }

    // Parse resolution in format WxH (e.g., 1920x1080)
    static bool tryParseResolution(const std::string& arg, unsigned int& widthOut, unsigned int& heightOut)
    {
        try
        {
            size_t xPos = arg.find('x');
            if (xPos == std::string::npos) return false;

            widthOut = std::stoi(arg.substr(0, xPos));
            heightOut = std::stoi(arg.substr(xPos + 1));
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to parse resolution: " << e.what() << std::endl;
            widthOut = 0;
            heightOut = 0;
            return false;
        }
    }

    // Function to process command-line arguments
    static bool processArgs(int argc, char* argv[], std::string& targetExeOut, std::string& targetDllOut)
    {
        // Check if we have the right number of arguments
        if (argc != TOTAL_ARGS)
        {
            std::cerr << "Invalid total args, expected " << TOTAL_ARGS << " got " << argc << std::endl;
            return false;
        }

        // Retrieve and validate the executable and DLL paths
        targetExeOut = argv[ARG_EXE];
        targetDllOut = argv[ARG_DLL];

        if (targetExeOut.empty() || targetDllOut.empty())
        {
            std::cerr << "Invalid target exe/dll" << std::endl;
            return false;
        }

        // Check if the files actually exist
        std::ifstream exeFile(targetExeOut);
        std::ifstream dllFile(targetDllOut);
        if (!exeFile.good() || !dllFile.good())
        {
            std::cerr << "Target exe/dll not found" << std::endl;
            return false;
        }

        // Check args are not empty
        if (std::string(argv[ARG_REAL_TIME]).empty()
            || std::string(argv[ARG_FRAME_SKIP]).empty()
            || std::string(argv[ARG_RESOLUTION]).empty()
            || std::string(argv[ARG_LOG_DIR_PATH]).empty()
            )
        {
            std::cerr << "empty args: real_time/frame_skip/resolution/log_dir" << std::endl;
            return false;
        }

        return true;
    }
}
