#include "highway-pursuit-launcher.hpp"
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
            bool isRealTime;
            std::istringstream(std::string(argv[ARG_REAL_TIME])) >> std::boolalpha >> isRealTime;

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
            bool renderEnabled;
            std::istringstream(std::string(argv[ARG_ENABLE_RENDERING])) >> std::boolalpha >> renderEnabled;

            // Log the parameters TODO: how to pass to the dll
            std::cout << "Executable: " << targetExe << "\n";
            std::cout << "DLL: " << targetDll << "\n";
            std::cout << "Real-time: " << isRealTime << "\n";
            std::cout << "Frame skip: " << frameSkip << "\n";
            std::cout << "Resolution: " << renderWidth << "x" << renderHeight << "\n";
            std::cout << "Render enabled: " << renderEnabled << "\n";

            // Inject the DLL into the target process
            injectDLL(targetExe, targetDll);

        }
        catch (const std::exception& e)
        {
            std::cerr << "Injection failed: " << e.what() << std::endl;
            return ExitCode::InjectionFailed;
        }

        return ExitCode::Success;
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

        // Additional argument validation can go here
        if (std::string(argv[ARG_REAL_TIME]).empty() || std::string(argv[ARG_FRAME_SKIP]).empty() || std::string(argv[ARG_RESOLUTION]).empty())
        {
            std::cerr << "empty args: real_time/frame_skip/resolution" << std::endl;
            return false;
        }

        return true;
    }

    // Example InjectDLL function (you need to implement the actual injection here)
    static void injectDLL(const std::string& targetExe, const std::string& targetDll)
    {
        // This is a placeholder for DLL injection logic.
        // Implement your CreateRemoteThread/LoadLibraryA code or use MinHook, etc.

        std::cout << "Injecting " << targetDll << " into " << targetExe << std::endl;
        // Call your injection logic here
        // Example: CreateRemoteThread / LoadLibraryA etc.
    }
}
