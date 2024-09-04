using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using EasyHook;
using HighwayPursuitServer.Injected;

namespace HighwayPursuitServer.Server
{
    class GameManager : IHookManager
    {
        private const string _d3d8ModuleName = "d3d8.dll";
        private const string _dinputModuleName = "DINPUT8.dll";

        private readonly Action<string> Report;
        private readonly IntPtr _moduleBase;
        private readonly IntPtr _d3d8Base;
        private readonly IntPtr _dinputBase;
        private readonly List<LocalHook> _hooks = new List<LocalHook>();
        private readonly UpdateService _updateService;
        private readonly ScoreService _scoreService;
        private readonly CheatService _cheatService;
        private readonly Direct3D8Service _direct3D8Service;
        private readonly InputService _inputService;
        private readonly Semaphore _lockUpdatePool; // Update thread waits for this
        private readonly Semaphore _lockServerPool; // Server thread waits for this

        public GameManager(Action<string> reportCallback)
        {
            // Init the update semaphore
            _lockUpdatePool = new Semaphore(initialCount: 1, maximumCount: 1);
            _lockServerPool = new Semaphore(initialCount: 0, maximumCount: 1);

            // Init the report function
            Report = reportCallback;

            // Modules for custom functions
            _moduleBase = Process.GetCurrentProcess().MainModule.BaseAddress;

            // D3D8 & DINPUT
            _d3d8Base = new IntPtr(0);
            _dinputBase = new IntPtr(0);
            foreach (ProcessModule module in Process.GetCurrentProcess().Modules)
            {
                if (module.ModuleName.Equals(_d3d8ModuleName))
                {
                    _d3d8Base = module.BaseAddress;
                } else if (module.ModuleName.Equals(_dinputModuleName))
                {
                    _dinputBase = module.BaseAddress;
                }
            }
            if (_d3d8Base.ToInt32() == 0) throw new Exception($"Couldn't find {_d3d8ModuleName}.");
            if (_dinputBase.ToInt32() == 0) throw new Exception($"Couldn't find {_dinputModuleName}.");

            // Init services & hooks
            const float FPS = 60.0f;
            const long PCFrequency = 1000000;
            _updateService = new UpdateService(this, _lockServerPool, _lockUpdatePool, FPS, PCFrequency);
            _scoreService = new ScoreService(this);
            _cheatService = new CheatService(this);
            _direct3D8Service = new Direct3D8Service(this);
            _inputService = new InputService(this);

            // Activate hooks on all threads except the current thread
            ActivateHooks();

            // Create the server thread
            CancellationTokenSource cts = new CancellationTokenSource();
            Task mainServerTask = Task.Run(() => ServerLoop(cts));
            AppDomain.CurrentDomain.ProcessExit += (sender, e) =>
            {
                cts.Cancel();
            };
        }

        private void ServerLoop(CancellationTokenSource cts)
        {
            int loopCount = 0;
            int startTick = Environment.TickCount;
            while (!cts.IsCancellationRequested)
            {
                _lockServerPool.WaitOne();
                _updateService.Step();
                try
                {
                    _direct3D8Service.Screenshot();
                } catch(D3DERR e)
                {
                    Report(e.Message);
                }

                // Time measurement stuff
                loopCount++;
                if (loopCount % 100 == 0)
                {
                    long elapsedTicks = Environment.TickCount - startTick;
                    var tps = 100.0 / (elapsedTicks / 1000.0);
                    var ratio = tps / 60.0;
                    Report($"ticks/s: {tps:0} = x{ratio:0.#}");
                    startTick = Environment.TickCount;
                }

                _lockUpdatePool.Release();
            }
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
        public Action<string> GetLoggingFunction()
        {
            return Report;
        }
    }
}
