#include "pch.h"
#include <iostream>
#include <filesystem>


bool HPLogger::_init = false;
std::string HPLogger::logDirectory;
std::string HPLogger::logFilePath;
const int HPLogger::maxLogFiles = 10;
const std::string HPLogger::logFileName = "log_";
const std::string HPLogger::logPattern = "log_*";

std::mutex HPLoggerLock;

void HPLogger::SetLogDir(const std::string& logDirectoryPath)
{
    if (!std::filesystem::exists(logDirectoryPath))
    {
        std::filesystem::create_directory(logDirectoryPath);
    }
    logDirectory = logDirectoryPath;

    // Create a string stream to format the date and time
    std::time_t now = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &now);
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y%m%d_%H%M%S");
    std::string timestamp = oss.str();

    logFilePath = logDirectory + "/" + logFileName + timestamp + ".txt";
    DeleteOldLogs();
    _init = true;
}

void HPLogger::LogDebug(const std::string& message)
{
    Log(message, "[DEBUG]");
}

void HPLogger::LogInfo(const std::string& message)
{
    Log(message, "[INFO]");
}

void HPLogger::LogWarning(const std::string& message)
{
    Log(message, "[WARNING]");
}

void HPLogger::LogError(const std::string& message)
{
    Log(message, "[ERROR]");
}

void HPLogger::LogException(const std::exception& exception)
{
    if (!_init) return;
    Log("Exception: " + std::string(exception.what()), "[ERROR]");
}

std::string HPLogger :: ToHex(int value)
{
    std::stringstream stream;
    stream << "0x"
        << std::hex << std::uppercase << value;
    return stream.str();
}

void HPLogger::Log(const std::string& message, const std::string& level)
{
    if (!_init) return;

    std::lock_guard<std::mutex> lock(HPLoggerLock);
    std::ofstream writer(logFilePath, std::ios::app);
    if (writer.is_open())
    {
        std::time_t now = std::time(nullptr);
        std::tm localTime;
        localtime_s(&localTime, &now);
        writer << "[" << level << "] " << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
            << " - " << message << std::endl;
    }
}

void HPLogger::DeleteOldLogs()
{
    std::vector<std::string> logFiles;
    for (const auto& entry : std::filesystem::directory_iterator(logDirectory))
    {
        if (entry.is_regular_file() && entry.path().string().find(logPattern) != std::string::npos)
        {
            logFiles.push_back(entry.path().string());
        }
    }
    std::sort(logFiles.begin(), logFiles.end(), std::greater<std::string>());

    for (size_t i = maxLogFiles; i < logFiles.size(); ++i)
    {
        std::filesystem::remove(logFiles[i]);
    }
}
