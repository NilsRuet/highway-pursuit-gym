#pragma once
#pragma pack(push, 1)
class HighwayPursuitArgs
{
    public:
        static const size_t prefixMaxSize = 256;

        bool isRealTime;
        int frameSkip;
        int renderWidth;
        int renderHeight;
        bool renderEnabled;
        char sharedResourcesPrefix[prefixMaxSize];

        HighwayPursuitArgs()
            : isRealTime(false),
            frameSkip(0),
            renderWidth(0),
            renderHeight(0),
            renderEnabled(false)
        {
            this->sharedResourcesPrefix[0] = '\0';
        }
            

        HighwayPursuitArgs(bool realTime, int skip, int width, int height, bool enableRender, const char* sharedResources)
            : isRealTime(realTime),
            frameSkip(skip),
            renderWidth(width),
            renderHeight(height),
            renderEnabled(enableRender)
        {
            strncpy_s(this->sharedResourcesPrefix, sharedResources, prefixMaxSize - 1);
            this->sharedResourcesPrefix[prefixMaxSize - 1] = '\0';  // Avoid buffer overflow
        }
};
#pragma pack(pop)