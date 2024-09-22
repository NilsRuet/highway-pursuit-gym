#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <mutex>
#include <filesystem>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <vector>


class HPLogger
{
public:
    static void SetLogDir(const std::string& logDirectoryPath);
    static void LogDebug(const std::string& message);
    static void LogInfo(const std::string& message);
    static void LogWarning(const std::string& message);
    static void LogError(const std::string& message);
    static void LogException(const std::exception& exception);
    static std::string ToHex(int value);

private:
    static bool _init;
    static std::string logDirectory;
    static std::string logFilePath;
    static const int maxLogFiles;
    static const std::string logFileName;
    static const std::string logPattern;

    static void Log(const std::string& message, const std::string& level);
    static void DeleteOldLogs();
};
