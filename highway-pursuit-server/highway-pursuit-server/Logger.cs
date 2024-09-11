using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer
{
    public static class Logger
    {
        private static readonly int maxLogFiles = 10;
        private static readonly string logFileName = $"log_{DateTime.Now:yyyyMMdd_HHmmss}.txt";
        private static readonly string logPattern = "log_*.txt";
        private static string logDirectory;
        private static string logFilePath;
        private static bool _init = false;

        public static void SetLogDir(string logDirectoryPath)
        {
            try
            {
                // Ensure log directory exists
                var fullPath = Path.GetFullPath(logDirectoryPath);
                if (!Directory.Exists(fullPath))
                {
                    Directory.CreateDirectory(fullPath);
                }
                logDirectory = logDirectoryPath;
                logFilePath = Path.Combine(logDirectoryPath, logFileName);
                DeleteOldLogs();
                _init = true;
            }
            catch (Exception)
            {
            }
        }

        private static readonly object loggerLock = new object();

        public enum Level
        {
            Debug,
            Info,
            Warning,
            Error
        }

        public static void Log(string message, Level level)
        {
            if (!_init) return;
#if !DEBUG
            if(level == Level.Debug)
            {
                return;
            }
#endif
            try
            {
                lock (loggerLock)
                {
                    using (StreamWriter writer = new StreamWriter(logFilePath, true))
                    {
                        writer.WriteLine($"[{level}] {DateTime.Now:yyyy-MM-dd HH:mm:ss} - {message}");
                    }
                }
            }
            catch (Exception)
            {
                // Don't crash if logging fails
            }
        }

        public static void LogException(Exception exception)
        {
            if (!_init) return;
            try
            {
                lock (loggerLock)
                {
                    using (StreamWriter writer = new StreamWriter(logFilePath, true))
                    {
                        writer.WriteLine($"[{Level.Error}] {DateTime.Now:yyyy-MM-dd HH:mm:ss} - [Exception] {exception.Message}");
                        writer.WriteLine($"Stack Trace: {exception.StackTrace}");

                        // Log any inner exceptions recursively
                        var innerException = exception.InnerException;
                        while (innerException != null)
                        {
                            writer.WriteLine($"Inner Exception: {innerException.Message}");
                            writer.WriteLine($"Inner Stack Trace: {innerException.StackTrace}");
                            innerException = innerException.InnerException;
                        }
                    }
                }
            }
            catch (Exception)
            {
                // Don't crash if logging fails
            }
        }

        private static void DeleteOldLogs()
        {
            // Get all log files in the directory
            var logFiles = Directory.GetFiles(logDirectory, logPattern)
                                    .OrderByDescending(f => f)
                                    .ToList();

            // Keep only the latest files, delete the rest
            for (int i = maxLogFiles - 1; i < logFiles.Count; i++)
            {
                File.Delete(logFiles[i]);
            }
        }
    }
}
