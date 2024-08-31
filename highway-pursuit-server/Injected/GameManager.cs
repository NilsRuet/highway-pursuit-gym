using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using EasyHook;

namespace HighwayPursuitServer.Injected
{
    class GameManager : IHookManager
    {
        private const string _d3d8ModuleName = "d3d8.dll";

        private readonly Action<string> Report;
        private readonly IntPtr _moduleBase;
        private readonly IntPtr _d3d8Base;
        private readonly List<LocalHook> _hooks = new List<LocalHook>();
        private readonly UpdateService _updateService;
        private readonly Direct3D8Service _direct3D8Service;
        private readonly CheatService _cheatService;

        public GameManager(Action<string> reportCallback)
        {
            // Init the report function
            Report = reportCallback;

            // Modules for custom functions
            _moduleBase = Process.GetCurrentProcess().MainModule.BaseAddress;

            // D3D8
            _d3d8Base = new IntPtr(0);
            foreach (ProcessModule module in Process.GetCurrentProcess().Modules)
            {
                if (module.ModuleName.Equals(_d3d8ModuleName))
                {
                    _d3d8Base = module.BaseAddress;
                    break;
                }
            }
            if (_d3d8Base.ToInt32() == 0) throw new Exception($"Couldn't find {_d3d8ModuleName}.");

            // Init services & hooks
            const float FPS = 60.0f;
            const long PCFrequency = 1000000;
            _updateService = new UpdateService(this, FPS, PCFrequency);
            _cheatService = new CheatService(this);
            _direct3D8Service = new Direct3D8Service(this);

            // Activate hooks on all threads except the current thread
            ActivateHooks();
        }

        private void ActivateHooks()
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
            EasyHook.LocalHook.Release();
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
    }
}
