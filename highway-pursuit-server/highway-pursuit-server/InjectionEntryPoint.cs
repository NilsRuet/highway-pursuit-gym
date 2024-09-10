using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;
using HighwayPursuitServer.Server;
using HighwayPursuitServer.Data;
using System.Threading;

namespace HighwayPursuitServer
{
    public class InjectionEntryPoint : EasyHook.IEntryPoint
    {
        public InjectionEntryPoint(EasyHook.RemoteHooking.IContext context, ServerOptions options)
        {
        }

        public void Run(EasyHook.RemoteHooking.IContext context, ServerOptions options)
        {
            bool processWokenUp = false;
            try
            {
                var comManager = new CommunicationManager(options);
                var server = new Server.HighwayPursuitServer(comManager, options);
                EasyHook.RemoteHooking.WakeUpProcess();
                processWokenUp = true;
                // Wait for the server, hooks are disabled if the main thread ends
                server.serverTask.Wait();
            }
            catch (Exception e)
            {
                if (!processWokenUp)
                {
                    // This has to be called in all cases otherwise the process might stay alive indefinitely
                    EasyHook.RemoteHooking.WakeUpProcess();
                    Logger.LogException(e);
                }
            }
        }
    }
}
