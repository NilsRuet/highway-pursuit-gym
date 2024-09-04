using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Server
{
    interface IHookManager
    {
        IntPtr GetModuleBase();
        IntPtr GetD3D8Base();
        IntPtr GetDINPUTBase();
        void RegisterHook(IntPtr address, Delegate hook, object inCallback = null);
        Action<string> GetLoggingFunction();
    }
}
