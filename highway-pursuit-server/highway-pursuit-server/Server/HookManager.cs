using EasyHook;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Server
{
    class HookManager: IHookManager
    {
        private const string _d3d8ModuleName = "d3d8.dll";
        private const string _dinputModuleName = "DINPUT8.dll";
        private readonly IntPtr _moduleBase;
        private readonly IntPtr _d3d8Base;
        private readonly IntPtr _dinputBase;
        private readonly List<LocalHook> _hooks = new List<LocalHook>();

        public HookManager()
        {
            using (Process proc = Process.GetCurrentProcess())
            {
                // Modules for custom functions
                _moduleBase = proc.MainModule.BaseAddress;
                // D3D8 & DINPUT
                _d3d8Base = new IntPtr(0);
                _dinputBase = new IntPtr(0);
                foreach (ProcessModule module in proc.Modules)
                {
                    if (module.ModuleName.Equals(_d3d8ModuleName))
                    {
                        _d3d8Base = module.BaseAddress;
                    }
                    else if (module.ModuleName.Equals(_dinputModuleName))
                    {
                        _dinputBase = module.BaseAddress;
                    }
                }
                if (_d3d8Base.ToInt32() == 0) throw new Exception($"Couldn't find {_d3d8ModuleName}.");
                if (_dinputBase.ToInt32() == 0) throw new Exception($"Couldn't find {_dinputModuleName}.");
            }
        }

        public void EnableHooks()
        {
            foreach (var hook in _hooks)
            {
                hook.ThreadACL.SetExclusiveACL(new int[] { 0 });
            }
        }

        public void Release()
        {
            // Remove hooks
            foreach (var hook in _hooks)
            {
                hook.Dispose();
            }

            // Finalise cleanup of hooks
            LocalHook.Release();
        }

        public void RegisterHook(IntPtr address, Delegate hookDelegate, object inCallback = null)
        {
            var hook = LocalHook.Create(address, hookDelegate, inCallback);
            _hooks.Add(hook);
        }

        public IntPtr GetModuleBase()
        {
            return _moduleBase;
        }

        public IntPtr GetD3D8Base()
        {
            return _d3d8Base;
        }

        public IntPtr GetDINPUTBase()
        {
            return _dinputBase;
        }
    }
}
