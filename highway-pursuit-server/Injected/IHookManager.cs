using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    interface IHookManager
    {
        IntPtr GetModuleBase();
        IntPtr GetD3D8Base();
        void RegisterHook(IntPtr address, Delegate hook, object inCallback = null);
    }
}
