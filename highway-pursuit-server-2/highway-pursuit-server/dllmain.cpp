// dllmain.cpp : Définit le point d'entrée de l'application DLL.
#include "pch.h"
#include "shared/HighwayPursuitArgs.hpp"

// This is an exported function that will be called by the injector
extern "C" __declspec(dllexport) void EntryPoint(LPVOID lpParam)
{
    HighwayPursuitArgs args = HighwayPursuitArgs();
    std::memcpy(&args, lpParam, sizeof(HighwayPursuitArgs));
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

