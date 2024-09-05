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
        private readonly EpisodeService _episodeService;
        private readonly UpdateService _updateService;
        private readonly InputService _inputService;
        private readonly Direct3D8Service _direct3D8Service;
        private readonly ScoreService _scoreService;
        private readonly CheatService _cheatService;
        private readonly Semaphore _lockUpdatePool; // Update thread waits for this
        private readonly Semaphore _lockServerPool; // Server thread waits for this

        const int NO_REWARD_TIMEOUT = 60 * 45; // No rewards for 45 seconds result in a reset (softlock safeguard)

        public GameManager(Action<string> reportCallback)
        {
            // Init the update semaphore
            // Allow one game update, zero server update as the initial state
            _lockUpdatePool = new Semaphore(initialCount: 1, maximumCount: 1);
            _lockServerPool = new Semaphore(initialCount: 0, maximumCount: 1);

            // Init the report function
            Report = reportCallback;

            // Init services & hooks
            const float FPS = 60.0f;
            const long PCFrequency = 1000000;
            _hookManager = new HookManager(Report);
            _episodeService = new EpisodeService(_hookManager);
            _updateService = new UpdateService(_hookManager, _lockServerPool, _lockUpdatePool, FPS, PCFrequency);
            _inputService = new InputService(_hookManager);
            _direct3D8Service = new Direct3D8Service(_hookManager);
            _scoreService = new ScoreService(_hookManager);
            _cheatService = new CheatService(_hookManager);

            // Activate hooks on all threads except the current thread
            _hookManager.EnableHooks();

            // Create the server thread
            CancellationTokenSource cts = new CancellationTokenSource();
            Task mainServerTask = Task.Run(() => ServerLoop(cts));
            AppDomain.CurrentDomain.ProcessExit += (sender, e) =>
            {
                _hookManager.Release();
                cts.Cancel();
            };
        }

        private void ServerLoop(CancellationTokenSource cts)
        {
            const int reportPeriod = 1000;
            int step = 0;
            int lastRewardedStep = 0;
            int startTick = Environment.TickCount;

            // Here we wait for update to be called at least once
            // This ensure the game is initialized
            // We can then start a new game
            if (!cts.IsCancellationRequested)
            {
                _lockServerPool.WaitOne();
                _updateService.Step();
                _episodeService.NewGame();
                _lockUpdatePool.Release();
            }

            // Main server loop
            while (!cts.IsCancellationRequested)
            {
                // Wait for the game to update
                _lockServerPool.WaitOne();
                // Setup actions TODO: load inputs from policy
                _inputService.SetInput(new List<Input> { });

                // Get game state
                try
                {
                    _direct3D8Service.Screenshot(); //TODO: how to collect/send the data?
                }
                catch (D3DERR e)
                {
                    Report(e.Message);
                }

                // Get reward
                var reward = _scoreService.PullReward();
                if (reward != 0)
                {
                    lastRewardedStep = step;
                }

                // Setup time for the next time step
                _updateService.Step();
                step++;

                // Softlock safeguard: reset if rewards stop being collected
                if (step - lastRewardedStep > NO_REWARD_TIMEOUT)
                {
                    _episodeService.NewLife();
                    lastRewardedStep = step;
                }

                // Performance metrics
                if (step % reportPeriod == 0)
                {
                    long elapsedTicks = Environment.TickCount - startTick;
                    var tps = ((float)reportPeriod) / (elapsedTicks / 1000.0);
                    var ratio = tps / 60.0;
                    Report($"ticks/s: {tps:0} = x{ratio:0.#}");
                    startTick = Environment.TickCount;
                }

                // Allow game loop update
                _lockUpdatePool.Release();
            }
        }
    }
}
