#include "pch.h"
#include "Shared/HighwayPursuitArgs.hpp"
#include "HighwayPursuitServer.hpp"
using namespace Shared;

// Called by the injector to initialize all hooks/services
extern "C" __declspec(dllexport) void Initialize(LPVOID lpParam)
{
    HighwayPursuitArgs args = HighwayPursuitArgs();
    std::memcpy(&args, lpParam, sizeof(HighwayPursuitArgs));
    //TODO: all hooks will be injected in this function
    std::string msg = "Initialize " + std::string(args.logDirPath);
    MessageBoxA(NULL, msg.c_str(), "DLL Notification", MB_OK);
    Data::ServerParams::RenderParams renderParams(args.renderWidth, args.renderHeight, args.renderEnabled);
    Data::ServerParams options(args.isRealTime, args.frameSkip, renderParams, args.logDirPath, args.sharedResourcesPrefix);
    HighwayPursuitServer server(options);
}

// Called by the injector to run the server once the process has been woken up
extern "C" __declspec(dllexport) void Run(LPVOID lpParam)
{
    //TODO: the main server thread will start here
    MessageBoxA(NULL, "Run", "DLL Notification", MB_OK);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            // Initialization
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            // Handle thread events
            break;
        case DLL_PROCESS_DETACH:
            // Cleanup code
            break;
	}
    return TRUE;
}

