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
            // TODO: a logging system!
        }

        public void Run(EasyHook.RemoteHooking.IContext context, ServerOptions options)
        {
            var comManager = new CommunicationManager(options);
            var server = new Server.HighwayPursuitServer(comManager, options);
            EasyHook.RemoteHooking.WakeUpProcess();

            // Wait for the server, hooks are disabled if the main thread ends
            server.serverTask.Wait();
        }
    }
}
