#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include "HighwayPursuitArgs.hpp"

namespace Injection
{
    std::string GetFullPath(const std::string& relativePath);
    bool CreateAndInject(const std::string& targetExe, const std::string& targetDll, const HighwayPursuitArgs& args);
}
