using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using HighwayPursuitServer.Injected;

namespace HighwayPursuitServer.Server
{
    class HighwayPursuitServer
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

        // Game options
        const float FPS = 60.0f;
        private readonly float TICKS_PER_FRAME = Stopwatch.Frequency / FPS;
        private readonly float TICKS_PER_MS = Stopwatch.Frequency / 1000.0f;
        const long PERFORMANCE_COUNTER_FREQUENCY = 1000000;
        const int NO_REWARD_TIMEOUT = 60 * 45; // No rewards for 45 seconds result in a reset (softlock safeguard)
        const bool IS_REAL_TIME = false;
        
        // Metrics options
        const int REPORT_PERIOD = 1000;

        private long _step; // current step
        private long _lastRewardedStep; // last step where reward wans't zero
        long startTick; // used to measure performance

        public HighwayPursuitServer(Action<string> reportCallback)
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
            Task mainServerTask = Task.Run(() => ServerThread(cts));
            AppDomain.CurrentDomain.ProcessExit += (sender, e) =>
            {
                _hookManager.Release();
                cts.Cancel();
            };
        }

        private void ServerThread(CancellationTokenSource cts)
        {
            startTick = Environment.TickCount;

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

            while (!cts.IsCancellationRequested)
            {
                if (IS_REAL_TIME)
                {
                    RealTimeLoop();
                } else
                {
                    FastLoop();
                }
            }
        }

        // Fast game update
        private void FastLoop()
        {
            HandleCurrentStep();
        }

        // Game update that is lengthened to last one frame of the specified FPS
        private void RealTimeLoop()
        {
            Stopwatch stopwatch = Stopwatch.StartNew();
            HandleCurrentStep();
            double sleepTime = (TICKS_PER_FRAME - stopwatch.ElapsedTicks) / TICKS_PER_MS;
            const int sleepToleranceMillisecond = 1;
            if (sleepTime > 0)
            {
                if (sleepTime > sleepToleranceMillisecond)
                {
                    Thread.Sleep((int)sleepTime - sleepToleranceMillisecond);
                }

                while (stopwatch.ElapsedTicks < TICKS_PER_FRAME)
                {
                    Thread.SpinWait(1);
                }
            }
            stopwatch.Stop();
        }

        // Frame code
        private void HandleCurrentStep()
        {
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
                _lastRewardedStep = _step;
            }

            // Answer with next state/reward
            // TODO

            // Next step
            _updateService.Step();
            _step++;

            HandleSoftlock();
            HandleMetrics();

            // Release game update
            _lockUpdatePool.Release();
        }

        // Safeguard against "softlock" states that may happens
        private void HandleSoftlock()
        {
            if (_step - _lastRewardedStep > NO_REWARD_TIMEOUT)
            {
                _episodeService.NewLife();
                _lastRewardedStep = _step;
            }
        }

        // Tracks some useful metrics for debugging
        private void HandleMetrics()
        {
            // Performance metrics
            if (_step % REPORT_PERIOD == 0)
            {
                long elapsedTicks = Environment.TickCount - startTick;
                var tps = ((float)REPORT_PERIOD) / (elapsedTicks / 1000.0);
                var ratio = tps / 60.0;
                Report($"ticks/s: {tps:0} = x{ratio:0.#}");
                startTick = Environment.TickCount;
            }
        }
    }
}
