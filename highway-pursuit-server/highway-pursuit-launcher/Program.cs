using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitLauncher
{
    class Program
    {
        const int ARG_COUNT = 10;
        const int SERVER_ARG_COUNT = ARG_COUNT - 2;
        const int SHARED_RESOURCES_ARGS_OFFSET = 3;

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
                string[] serverArgs = new string[SERVER_ARG_COUNT];
                Array.Copy(args, ARG_COUNT - SERVER_ARG_COUNT, serverArgs, 0, SERVER_ARG_COUNT);

                // start and inject into a new process
                EasyHook.RemoteHooking.CreateAndInject(
                    targetExe,
                    "",
                    0,
                    EasyHook.InjectionOptions.DoNotRequireStrongName,
                    targetDll,
                    targetDll,
                    out _,
                    serverArgs
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

            // isRealTime, arg[2], is not checked because it will be defaulted to false if needed 

            // Check resource names (all remaining args)
            for (int i = SHARED_RESOURCES_ARGS_OFFSET; i < args.Length; i++)
            {
                if (string.IsNullOrEmpty(args[i])) return false;
            }

            return true;
        }
    }
}
