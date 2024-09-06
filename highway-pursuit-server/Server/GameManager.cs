using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
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

        const float FPS = 60.0f;
        private readonly float TICKS_PER_FRAME = Stopwatch.Frequency / FPS;
        private readonly float TICKS_PER_MS = Stopwatch.Frequency / 1000.0f;
        const long PERFORMANCE_COUNTER_FREQUENCY = 1000000;
        const int NO_REWARD_TIMEOUT = 60 * 45; // No rewards for 45 seconds result in a reset (softlock safeguard)
        const bool IS_REAL_TIME = false;

        public GameManager(Action<string> reportCallback)
        {
            // Init the update semaphore
            // Allow one game update, zero server update as the initial state
            _lockUpdatePool = new Semaphore(initialCount: 1, maximumCount: 1);
            _lockServerPool = new Semaphore(initialCount: 0, maximumCount: 1);

            // Init the report function
            Report = reportCallback;

            // Init services & hooks
            _hookManager = new HookManager(Report);
            _episodeService = new EpisodeService(_hookManager);
            _updateService = new UpdateService(_hookManager, IS_REAL_TIME, _lockServerPool, _lockUpdatePool, FPS, PERFORMANCE_COUNTER_FREQUENCY);
            _inputService = new InputService(_hookManager);
            _direct3D8Service = new Direct3D8Service(_hookManager);
            _scoreService = new ScoreService(_hookManager);
            _cheatService = new CheatService(_hookManager);

            // Activate hooks on all threads except the current thread
            _hookManager.EnableHooks();

            // Create the server thread
            CancellationTokenSource cts = new CancellationTokenSource();
            Task mainServerTask = Task.Run(() => ServerLoop(cts, IS_REAL_TIME));
            AppDomain.CurrentDomain.ProcessExit += (sender, e) =>
            {
                _hookManager.Release();
                cts.Cancel();
            };
        }

        private void ServerLoop(CancellationTokenSource cts, bool isRealTime)
        {
            // TODO: probably create a "Server" class, to split frame handling into methods while keeping context
            const int reportPeriod = 1000;
            long step = 0;
            long lastRewardedStep = 0;
            long startTick = Environment.TickCount;

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

            // Handle loop
            if (!isRealTime)
            {
                while (!cts.IsCancellationRequested)
                {
                    handleFrame();
                }
            } else
            {
                while (!cts.IsCancellationRequested)
                {
                    Stopwatch stopwatch = Stopwatch.StartNew();
                    handleFrame();
                    double sleepTime = (TICKS_PER_FRAME - stopwatch.ElapsedTicks) / TICKS_PER_MS;
                    const int sleepToleranceMillisecond = 1;
                    if(sleepTime > 0)
                    {
                        if(sleepTime > sleepToleranceMillisecond)
                        {
                            Thread.Sleep((int)sleepTime - sleepToleranceMillisecond);
                        }

                        while(stopwatch.ElapsedTicks < TICKS_PER_FRAME)
                        {
                            Thread.SpinWait(1);
                        }
                    }
                    stopwatch.Stop();
                }
            }

            // Frame code
            void handleFrame() {
                // Wait for and lock game update
                _lockServerPool.WaitOne();

                // Receive action (or reset) from the gym env
                // TODO

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

                // Answer with next state/reward
                // TODO

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

                // Release game update
                _lockUpdatePool.Release();
            }
        }
    }
}
