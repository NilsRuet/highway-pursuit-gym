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
        const int ARG_COUNT = 10;
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
                    args[SERVER_ARGS_OFFSET + 1],
                    args[SERVER_ARGS_OFFSET + 2],
                    args[SERVER_ARGS_OFFSET + 3],
                    args[SERVER_ARGS_OFFSET + 4],
                    args[SERVER_ARGS_OFFSET + 5],
                    args[SERVER_ARGS_OFFSET + 6],
                    args[SERVER_ARGS_OFFSET + 7]
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
            if(args.Length != ARG_COUNT)
            {
                targetExe = null;
                targetDll = null;
                return false;
            }

            // Check paths
            targetExe = args[0];
            targetDll = args[1];
            if (string.IsNullOrEmpty(targetDll) || string.IsNullOrEmpty(targetDll))
            {
                return false;
            }

            if(!File.Exists(targetExe) || !File.Exists(targetDll))
            {
                return false;
            }

            // Check other args (isRealTime which is always defaulted, and the resource names)
            // TODO: maybe a warning is isRealTime is not a proper bool str?
            for (int i = SERVER_ARGS_OFFSET; i < args.Length; i++)
            {
                if (string.IsNullOrEmpty(args[i])) return false;
            }

            return true;
        }
    }
}
