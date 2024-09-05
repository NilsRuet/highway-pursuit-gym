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
    class GameManager
    {
        private readonly Action<string> Report;
        private readonly IHookManager _hookManager;
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

            // Init services & hooks
            const float FPS = 60.0f;
            const long PCFrequency = 1000000;
            _hookManager = new HookManager(Report);
            _updateService = new UpdateService(_hookManager, _lockServerPool, _lockUpdatePool, FPS, PCFrequency);
            _scoreService = new ScoreService(_hookManager);
            _cheatService = new CheatService(_hookManager);
            _direct3D8Service = new Direct3D8Service(_hookManager);
            _inputService = new InputService(_hookManager);

            // Activate hooks on all threads except the current thread
            _hookManager.EnableHooks();

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
            const int reportPeriod = 1000;
            int loopCount = 0;
            int startTick = Environment.TickCount;
            while (!cts.IsCancellationRequested)
            {
                _lockServerPool.WaitOne();
                _inputService.SetInput(new List<Input> { }); //TODO : load inputs from policy
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
                if (loopCount % reportPeriod == 0)
                {
                    long elapsedTicks = Environment.TickCount - startTick;
                    var tps = ((float)reportPeriod) / (elapsedTicks / 1000.0);
                    var ratio = tps / 60.0;
                    Report($"ticks/s: {tps:0} = x{ratio:0.#}");
                    startTick = Environment.TickCount;
                }

                _lockUpdatePool.Release();
            }
        }
    }
}
