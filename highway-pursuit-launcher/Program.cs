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
        static void Main(string[] args)
        {
            string channelName = null;

            ProcessArgs(args, out string targetExe);

            if (string.IsNullOrEmpty(targetExe)) return;

            // Create the IPC server using the FileMonitorIPC.ServiceInterface class as a singleton
            EasyHook.RemoteHooking.IpcCreateServer<HighwayPursuitServer.ServerInterface>(ref channelName, System.Runtime.Remoting.WellKnownObjectMode.Singleton);

            string injectionLibrary = Path.Combine(Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location), "HighwayPursuitServer.dll");

            try
            {
                Console.WriteLine("Attempting to create and inject into {0}", targetExe);
                // start and inject into a new process
                EasyHook.RemoteHooking.CreateAndInject(
                    targetExe,
                    "",
                    0,
                    EasyHook.InjectionOptions.DoNotRequireStrongName,
                    injectionLibrary,
                    injectionLibrary,
                    out _,
                    channelName
                );
            }
            catch (Exception e)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("There was an error while injecting into target:");
                Console.ResetColor();
                Console.WriteLine(e.ToString());
            }
            Console.WriteLine("Success!");
            Console.WriteLine("<Press any key to exit>");
            Console.ResetColor();
            Console.ReadKey();
        }

        static void ProcessArgs(string[] args, out string targetExe)
        {
            targetExe = null;
            Console.WriteLine("Starting Highway pursuit...");

            // Load any parameters
            if (args.Length != 1 || !File.Exists(args[0]))
            {
                Console.WriteLine("Usage: HighwayPursuitLauncher PathToExecutable");
            }
            else
            {
                targetExe = args[0];
            }
        }
    }
}
