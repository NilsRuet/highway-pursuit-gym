using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;
using HighwayPursuitServer.Server;

namespace HighwayPursuitServer
{
    public class InjectionEntryPoint : EasyHook.IEntryPoint
    {
        public InjectionEntryPoint(EasyHook.RemoteHooking.IContext context, params object[] args)
        {
            // TODO: a logging system!
        }

        public void Run(EasyHook.RemoteHooking.IContext context, params object[] args)
        {
            var manager = new Server.HighwayPursuitServer();
            EasyHook.RemoteHooking.WakeUpProcess();
        }
    }
}
