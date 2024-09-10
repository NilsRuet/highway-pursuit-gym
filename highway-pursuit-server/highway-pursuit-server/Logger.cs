using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer
{
    public static class Logger
    {
        private static readonly string logDirectory = "./logs/";
        private static readonly int maxSessions = 10;
        private static readonly string logFileName = $"{DateTime.Now:yyyyMMdd_HHmmss}.txt";
        private static readonly string logFilePath = Path.Combine(logDirectory, logFileName);
        private static readonly object loggerLock = new object();

        public enum Level
        {
            Debug,
            Info,
            Warning,
            Error
        }

        static Logger()
        {
            // Ensure log directory exists
            if (!Directory.Exists(logDirectory))
            {
                Directory.CreateDirectory(logDirectory);
            }

            DeleteOldLogs();
        }

        public static void Log(string message, Level level)
        {
#if !DEBUG
            if(level == Level.Debug)
            {
                return;
            }
#endif

            lock (loggerLock)
            {
                using (StreamWriter writer = new StreamWriter(logFilePath, true))
                {
                    writer.WriteLine($"[{level}] {DateTime.Now:yyyy-MM-dd HH:mm:ss} - {message}");
                }
            }
        }

        public static void LogException(Exception exception)
        {
            lock (loggerLock)
            {
                using (StreamWriter writer = new StreamWriter(logFilePath, true))
                {
                    writer.WriteLine($"[{Level.Error}] {DateTime.Now:yyyy-MM-dd HH:mm:ss} - Exception: {exception.Message}");
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

        private static void DeleteOldLogs()
        {
            // Get all log files in the directory
            var logFiles = Directory.GetFiles(logDirectory, "dll_log_*.txt")
                                    .OrderByDescending(f => f)
                                    .ToList();

            // Keep only the latest maxSessions files, delete the rest
            for (int i = maxSessions; i < logFiles.Count; i++)
            {
                File.Delete(logFiles[i]);
            }
        }
    }
}
