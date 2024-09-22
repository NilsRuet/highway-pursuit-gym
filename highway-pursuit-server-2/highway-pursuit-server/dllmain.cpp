#include "pch.h"
#include "Shared/HighwayPursuitArgs.hpp"
#include "HighwayPursuitServer.hpp"
using namespace Shared;

static std::unique_ptr<HighwayPursuitServer> serverPtr = nullptr;

// Called by the injector to initialize all hooks/services
extern "C" __declspec(dllexport) void Initialize(LPVOID lpParam)
{
    HighwayPursuitArgs args = HighwayPursuitArgs();
    std::memcpy(&args, lpParam, sizeof(HighwayPursuitArgs));

    // Setup logger
    HPLogger::SetLogDir(args.logDirPath);
    try
    {
        // Setup hooks
        Data::ServerParams::RenderParams renderParams(args.renderWidth, args.renderHeight, args.renderEnabled);
        Data::ServerParams options(args.isRealTime, args.frameSkip, renderParams, args.sharedResourcesPrefix);
        serverPtr = std::make_unique<HighwayPursuitServer>(options);
    }
    catch (const std::runtime_error& e)
    {
        HPLogger::LogError(e.what());
    }
}

// Called by the injector to run the server once the process has been woken up
extern "C" __declspec(dllexport) void Run(LPVOID lpParam)
{
    try
    {
        serverPtr->Run();
    }
    catch (const std::runtime_error& e)
    {
        HPLogger::LogError(e.what());
    }
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

