using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using HighwayPursuitServer.Data;
using HighwayPursuitServer.Exceptions;
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
        const int GAME_TIMEOUT = 2000; // results in an error if the game fails to update
        const int NO_REWARD_TIMEOUT = 60 * 45; // No rewards for 45 seconds result in a reset (softlock safeguard)
        const int LOG_PERIOD = 60 * 60; // update metrics every minute of gameplay
        // Instance members
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


        private bool _firstEpisodeInitialized = false;
        private bool _serverTerminated = false;
        private long _step; // current step
        private long _totalSteps;
        private long _lastRewardedStep; // last step where reward wasn't zero
        private Termination _lastStepTermination = new Termination(false, false);
        private long startTick; // used to measure performance


        public HighwayPursuitServer(CommunicationManager communicationManager, ServerOptions options)
        {
            _communicationManager = communicationManager; // This will connect once the game is initialized
            _options = options;

            // Setup interactions with the game
            _lockUpdatePool = new Semaphore(initialCount: 0, maximumCount: 1);
            _lockServerPool = new Semaphore(initialCount: 0, maximumCount: 1);

            // Init services & hooks
            _hookManager = new HookManager();
            _episodeService = new EpisodeService(_hookManager);
            _updateService = new UpdateService(_hookManager, _options.isRealTime, _lockServerPool, _lockUpdatePool, FPS, PERFORMANCE_COUNTER_FREQUENCY);
            _inputService = new InputService(_hookManager);
            _direct3D8Service = new Direct3D8Service(_hookManager);
            _scoreService = new ScoreService(_hookManager);
            _cheatService = new CheatService(_hookManager);

            // Activate hooks on all threads except the current thread
            _hookManager.EnableHooks();
        }

        public void Run()
        {
            try
            {
                // Wait for game to be initialized
                WaitGameUpdate();
                SkipIntro();

                // Get server info now that the D3D device is initialized
                D3DDISPLAYMODE display = _direct3D8Service.GetDisplayMode();
                var serverInfo = new ServerInfo(
                    display.Height,
                    display.Width,
                    display.GetChannelCount(),
                    (uint)_inputService.GetInputCount()
                );

                // Setup the communication
                _communicationManager.Connect(serverInfo);

                startTick = Environment.TickCount;
                while (!_serverTerminated)
                {
                    _communicationManager.ExecuteOnInstruction(HandleInstruction);
                }
            }
            // Exception handling for the server thread
            catch (HighwayPursuitException e)
            {
                _communicationManager.WriteException(e);
                Logger.LogException(e);
            }
            catch (Exception e)
            {
                _communicationManager.WriteException(new HighwayPursuitException(ErrorCode.NATIVE_ERROR));
                Logger.LogException(e);
            }
            finally
            {
                _hookManager.Release();
                _communicationManager.Dispose();
            }
        }

        private void WaitGameUpdate()
        {
            _updateService.UpdateTime();
            _lockUpdatePool.Release();
            bool acquired = _lockServerPool.WaitOne(GAME_TIMEOUT);
            if (!acquired)
            {
                throw new HighwayPursuitException(ErrorCode.GAME_TIMEOUT);
            }
        }

        private void SkipIntro()
        {
            // Calling new game skips the intro
            // We then skip the initial zoom out, and wait 2 frames for the fade out the complete
            _episodeService.NewGame();
            _direct3D8Service.ResetZoomLevel();
            WaitGameUpdate();
            WaitGameUpdate();
        }

        private void HandleInstruction(InstructionCode code)
        {
            switch (code)
            {
                case InstructionCode.RESET:
                    Reset();
                    break;
                case InstructionCode.STEP:
                    // Return an error if step is called without reset (player dead)
                    if ((!_firstEpisodeInitialized) || _lastStepTermination.IsDone())
                    {
                        _communicationManager.WriteError(ErrorCode.ENVIRONMENT_NOT_RESET);
                    }

                    // Step at max speed or real time depending on the option
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
                    _serverTerminated = true;
                    // ensure the updates hooks are non-blocking
                    _updateService.DisableSemaphores();
                    _lockUpdatePool.Release();
                    break;
                default:
                    break;
            }
        }

        private void Reset()
        {
            // Reset the episode
            if (!_firstEpisodeInitialized)
            {
                // New game is called on the first episode because it fully resets the game state
                _episodeService.NewGame();
                _firstEpisodeInitialized = true;
            }
            // Respawn the player if last step has not done it naturally (if the client called reset without waiting for termination/truncation)
            else if (!_lastStepTermination.IsDone())
            {
                _episodeService.NewLife();
            }
            // Reset last termination

            // Wait for one frame for the rendering buffer to update
            WaitGameUpdate();

            // Update step variables
            _step = 0;
            _lastRewardedStep = 0;
            _lastStepTermination = new Termination(false, false);

            // Transfer state/info
            _direct3D8Service.Screenshot(_communicationManager.WriteObservationBuffer);
            _communicationManager.WriteInfoBuffer(new Info(1.0f, 2.0f));
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
            else
            {
                Logger.Log("Game update took longer than one frame.", Logger.Level.Warning);
            }
            stopwatch.Stop();
        }

        // Frame code
        private void Step()
        {
            // Get action
            List<Input> actions = _communicationManager.ReadActions();
            _inputService.SetInput(actions);

            // Apply action and get next state
            WaitGameUpdate();

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
            _totalSteps++;

            // Truncation : safeguard against softlocks
            bool truncated = false;
            if (_step - _lastRewardedStep > NO_REWARD_TIMEOUT)
            {
                truncated = true;
                _episodeService.Truncate();
            }

            // Check termination (death)
            bool terminated = _episodeService.PullTerminated();

            _lastStepTermination = new Termination(terminated: terminated, truncated: truncated);
            _communicationManager.WriteTerminationBuffer(_lastStepTermination);

            HandleLogs();
        }


        // Tracks some useful metrics for debugging
        private void HandleLogs()
        {
            // Performance metrics
            if (_totalSteps % LOG_PERIOD == 0)
            {
                long elapsedTicks = Environment.TickCount - startTick;
                var tps = ((float)LOG_PERIOD) / (elapsedTicks / 1000.0);
                var ratio = tps / 60.0;

                double memorySize = 0;
                using (Process proc = Process.GetCurrentProcess())
                {
                    memorySize = proc.PrivateMemorySize64 / (1024.0 * 1024.0);
                }

                Logger.Log($"step {_totalSteps} -> {tps:0} ticks/s = x{ratio:0.#} | RAM:{memorySize:0.##}Mb", Logger.Level.Debug);
                startTick = Environment.TickCount;
            }
        }
    }
}
