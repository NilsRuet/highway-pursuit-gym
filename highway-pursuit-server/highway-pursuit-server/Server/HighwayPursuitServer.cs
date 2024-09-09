using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using HighwayPursuitServer.Data;
using HighwayPursuitServer.Injected;

namespace HighwayPursuitServer.Server
{
    class HighwayPursuitServer
    {
        // Constants
        const float FPS = 60.0f;
        private readonly float TICKS_PER_FRAME = Stopwatch.Frequency / FPS;
        private readonly float TICKS_PER_MS = Stopwatch.Frequency / 1000.0f;
        const long PERFORMANCE_COUNTER_FREQUENCY = 1000000;
        const int NO_REWARD_TIMEOUT = 60 * 45; // No rewards for 45 seconds result in a reset (softlock safeguard)
        const int REPORT_PERIOD = 5 * 60 * 60; // update metrics every 5 minutes of gameplay

        // Instance members
        public readonly Task serverTask;
        private readonly ServerOptions _options; // Game options

        private readonly CommunicationManager _communicationManager;
        private readonly IHookManager _hookManager;
        private readonly EpisodeService _episodeService;
        private readonly UpdateService _updateService;
        private readonly InputService _inputService;
        private readonly Direct3D8Service _direct3D8Service;
        private readonly ScoreService _scoreService;
        private readonly CheatService _cheatService;
        private readonly Semaphore _lockUpdatePool; // Update thread waits for this
        private readonly Semaphore _lockServerPool; // Server thread waits for this

        private bool _terminated = false;
        private long _step; // current step
        private long _lastRewardedStep; // last step where reward wasn't zero
        private long startTick; // used to measure performance

        public HighwayPursuitServer(CommunicationManager communicationManager, ServerOptions options)
        {
            _communicationManager = communicationManager;
            _options = options;

            // Init the update semaphore
            // Allow one game update, zero server update as the initial state
            _lockUpdatePool = new Semaphore(initialCount: 1, maximumCount: 1);
            _lockServerPool = new Semaphore(initialCount: 0, maximumCount: 1);

            // Init services & hooks
            _hookManager = new HookManager(communicationManager.Log);
            _episodeService = new EpisodeService(_hookManager);
            _updateService = new UpdateService(_hookManager, _options.isRealTime, _lockServerPool, _lockUpdatePool, FPS, PERFORMANCE_COUNTER_FREQUENCY);
            _inputService = new InputService(_hookManager);
            _direct3D8Service = new Direct3D8Service(_hookManager);
            _scoreService = new ScoreService(_hookManager);
            _cheatService = new CheatService(_hookManager);

            // Activate hooks on all threads except the current thread
            _hookManager.EnableHooks();

            //Setup the communication
            _communicationManager.Connect();

            // Create the server thread
            CancellationTokenSource cts = new CancellationTokenSource();
            serverTask =
                Task.Run(() => ServerThread(cts))
                .ContinueWith((task) =>
                {
                    _hookManager.Release();
                    _communicationManager.Dispose();
                });

            // TODO: That might be useless
            AppDomain.CurrentDomain.ProcessExit += (sender, e) =>
            {
                if (!serverTask.IsCompleted)
                {
                    cts.Cancel();
                }
            };
        }

        private void WaitUpdateAndExecute(Action action)
        {
            _lockServerPool.WaitOne();
            try
            {
                action();
            }
            finally
            {
                _lockUpdatePool.Release();
            }
        }

        private void ServerThread(CancellationTokenSource cts)
        {
            // Wait for update to be called at least once
            // This ensures the game is initialized
            startTick = Environment.TickCount;
            if (!cts.IsCancellationRequested)
            {
                WaitUpdateAndExecute(() =>
                {
                    _updateService.UpdateTime();
                });
            }

            // Main loop
            while (!cts.IsCancellationRequested && !_terminated)
            {
                _communicationManager.ExecuteOnInstruction(HandleInstruction);
            }
        }

        private void HandleInstruction(InstructionCode code)
        {
            switch (code)
            {
                case InstructionCode.RESET:
                    Reset();
                    break;
                case InstructionCode.STEP:
                    if (_options.isRealTime)
                    {
                        StepRealTime();
                    }
                    else
                    {
                        StepSkipTime();
                    }
                    break;
                case InstructionCode.CLOSE:
                    // Notify end of loop
                    _terminated = true;
                    break;
                default:
                    break;
            }
        }

        private void Reset()
        {
            // Reset the episode
            WaitUpdateAndExecute(() =>
            {
                _episodeService.NewGame();
                _updateService.UpdateTime();
            });

            // Update step variables
            _step = 0;
            _lastRewardedStep = 0;

            // Wait for one frame for the new episode starting state
            WaitUpdateAndExecute(() =>
            {
                // Transfer state/info
                _direct3D8Service.Screenshot(_communicationManager.WriteObservationBuffer);
                _communicationManager.WriteInfoBuffer(new Info(1.0f, 2.0f));
            });
        }

        // Fast game update
        private void StepSkipTime()
        {
            Step();
        }

        // Game update that is lengthened to last one frame of the specified FPS
        private void StepRealTime()
        {
            Stopwatch stopwatch = Stopwatch.StartNew();
            Step();
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
        private void Step()
        {
            WaitUpdateAndExecute(
            () =>
            {
                // Get action
                // TODO: actions are effectively delayed. This doesn't take effect during the current transition but during the next one.
                List<Input> actions = _communicationManager.ReadActions();
                _inputService.SetInput(actions);

                // Get game state
                _direct3D8Service.Screenshot(_communicationManager.WriteObservationBuffer);

                // Get reward
                var reward = _scoreService.PullReward();
                _communicationManager.WriteRewardBuffer(new Reward(reward));
                if (reward != 0)
                {
                    _lastRewardedStep = _step;
                }

                // Next step
                _updateService.UpdateTime();
                _step++;

                HandleSoftlock();
                HandleMetrics();
            });
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

                double memorySize = 0;
                using (Process proc = Process.GetCurrentProcess())
                {
                    memorySize = proc.PrivateMemorySize64 / (1024.0 * 1024.0);
                }

                _hookManager.Log($"step {_step} -> {tps:0} ticks/s = x{ratio:0.#} | RAM:{memorySize:0.##}Mb");
                startTick = Environment.TickCount;
            }
        }
    }
}
