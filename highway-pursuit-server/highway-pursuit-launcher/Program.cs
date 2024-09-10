using HighwayPursuitServer.Data;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace HighwayPursuitLauncher
{
    class Program
    {
        const int ARG_EXE = 0;
        const int ARG_DLL = 1;
        const int ARG_REAL_TIME = 2;
        const int ARG_LOG_DIR_PATH = 3;
        const int ARG_SHARED_RESOURCES_PREFIX = 4;
        const int TOTAL_ARGS = 5;

        const int SERVER_ARGS_OFFSET = 2;

        enum ExitCode : int
        {
            Success = 0,
            InvalidArgs = 1,
            InjectionFailed = 2
        }

        static void Main(string[] args)
        {
            if (!ProcessArgs(args, out string targetExe, out string targetDll))
            {
                Environment.Exit((int)ExitCode.InvalidArgs);
            }

            try
            {
                var success = bool.TryParse(args[SERVER_ARGS_OFFSET+0], out bool isRealTime);
                if (!success)
                {
                    isRealTime = false;
                }

                var options = new ServerOptions(
                    isRealTime,
                    args[ARG_LOG_DIR_PATH],
                    args[ARG_SHARED_RESOURCES_PREFIX]
                );

                // start and inject into a new process
                EasyHook.RemoteHooking.CreateAndInject(
                    targetExe,
                    "",
                    0,
                    EasyHook.InjectionOptions.DoNotRequireStrongName,
                    targetDll,
                    targetDll,
                    out _,
                    options
                );
            }
            catch (Exception)
            {
                Environment.Exit((int)ExitCode.InjectionFailed);
            }
        }

        static bool ProcessArgs(string[] args, out string targetExe, out string targetDll)
        {

            // Check arg count
            if(args.Length != TOTAL_ARGS)
            {
                targetExe = null;
                targetDll = null;
                return false;
            }

            // Check paths
            targetExe = args[ARG_EXE];
            targetDll = args[ARG_DLL];
            if (string.IsNullOrEmpty(targetDll) || string.IsNullOrEmpty(targetDll))
            {
                return false;
            }

            if(!File.Exists(targetExe) || !File.Exists(targetDll))
            {
                return false;
            }

            if (string.IsNullOrEmpty(args[ARG_REAL_TIME]))
            {
                return false;
            }

            // Check shared resource name
            if (string.IsNullOrEmpty(args[ARG_SHARED_RESOURCES_PREFIX]))
            {
                return false;
            }

            return true;
        }
    }
}
