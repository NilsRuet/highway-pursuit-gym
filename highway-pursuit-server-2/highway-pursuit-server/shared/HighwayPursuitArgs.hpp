#pragma once
#include "../pch.h"

namespace Shared
{
    #pragma pack(push, 1)
    struct HighwayPursuitArgs
    {
        static const size_t prefixMaxSize = 256;

        bool isRealTime;
        int frameSkip;
        int renderWidth;
        int renderHeight;
        bool renderEnabled;
        char sharedResourcesPrefix[prefixMaxSize];
        char logDirPath[MAX_PATH];

        HighwayPursuitArgs()
            : isRealTime(false),
            frameSkip(0),
            renderWidth(0),
            renderHeight(0),
            renderEnabled(false)
        {
            this->logDirPath[0] = '\0';
            this->sharedResourcesPrefix[0] = '\0';
        }

        HighwayPursuitArgs(bool realTime, int skip, int width, int height, bool enableRender, const char* logDirPath, const char* sharedResources)
            : isRealTime(realTime),
            frameSkip(skip),
            renderWidth(width),
            renderHeight(height),
            renderEnabled(enableRender)
        {
            strncpy_s(this->logDirPath, logDirPath, MAX_PATH - 1);
            this->logDirPath[MAX_PATH - 1] = '\0';

            strncpy_s(this->sharedResourcesPrefix, sharedResources, prefixMaxSize - 1);
            this->sharedResourcesPrefix[prefixMaxSize - 1] = '\0';
        }
    };
    #pragma pack(pop)
}
