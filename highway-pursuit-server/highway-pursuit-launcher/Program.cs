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
        const int ARG_FRAME_SKIP = 3;
        const int ARG_RESOLUTION = 4;
        const int ARG_ENABLE_RENDERING = 5;
        const int ARG_LOG_DIR_PATH = 6;
        const int ARG_SHARED_RESOURCES_PREFIX = 7;
        const int TOTAL_ARGS = 8;

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
                var success = bool.TryParse(args[ARG_REAL_TIME], out bool isRealTime);
                if (!success)
                {
                    isRealTime = false;
                }

                if(!int.TryParse(args[ARG_FRAME_SKIP], out int frameskip) || frameskip < 1)
                {
                    frameskip = 1;
                }

                if(!TryParseResolution(args[ARG_RESOLUTION], out uint renderWidth, out uint renderHeight))
                {
                    renderWidth = 640;
                    renderHeight = 480;
                }

                if (!bool.TryParse(args[ARG_ENABLE_RENDERING], out bool renderEnabled))
                {
                    renderEnabled = true;
                }

                var renderParams = new ServerParams.RenderParams(renderWidth,renderHeight, renderEnabled);
                var options = new ServerParams(
                    isRealTime,
                    frameskip,
                    renderParams,
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

        static bool TryParseResolution(string arg, out uint width, out uint height)
        {
            try
            {
                var dims = arg.Split('x');
                width = uint.Parse(dims[0]);
                height = uint.Parse(dims[1]);
                return true;
            } catch
            {
                width = 0;
                height = 0;
                return false;
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

            if (string.IsNullOrEmpty(args[ARG_FRAME_SKIP]))
            {
                return false;
            }

            if (string.IsNullOrEmpty(args[ARG_RESOLUTION]))
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
