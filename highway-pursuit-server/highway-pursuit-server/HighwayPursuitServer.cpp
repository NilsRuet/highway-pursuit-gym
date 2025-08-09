#include "pch.h"
#include "HighwayPursuitServer.hpp"

HighwayPursuitServer::HighwayPursuitServer(const Data::ServerParams& options)
    : _options(options),
    _firstEpisodeInitialized(false),
    _serverTerminated(false),
    _totalEllapsedFrames(0),
    _lastStepTermination(false, false),
    _currentInfo(Data::Info(0.0f, 0.0f, 0.0f, 0.0f)),
    _startTick(0),
    _cumulatedServerTicks(0),
    _cumulatedGameTicks(0)
{
    // num/den is the period in seconds, den/num is therefore the frequency in hertz
    auto ticks_per_s = (static_cast<float>(std::chrono::high_resolution_clock::period::den) / std::chrono::high_resolution_clock::period::num);
    TICKS_PER_FRAME = ticks_per_s / FPS;
    TICKS_PER_MS = ticks_per_s / 1000.0f;

    // Hook manager
    _hookManager = std::make_shared<HookManager>();

    // init communication manager
    _communicationManager = std::make_unique<CommunicationManager>(options);

    // init update semaphores
    _lockUpdatePool = CreateSemaphore(nullptr, 0, 1, nullptr);
    _lockServerPool = CreateSemaphore(nullptr, 0, 1, nullptr);

    // Install static hooks/services
    LARGE_INTEGER frequency;
    frequency.QuadPart = PERFORMANCE_COUNTER_FREQUENCY;
    _windowService = std::make_unique<WindowService>(_hookManager);
    _episodeService = std::make_unique<EpisodeService>(_hookManager);
    _updateService = std::make_unique<UpdateService>(_hookManager, _options.isRealTime, _lockServerPool, _lockUpdatePool, FPS, frequency);
    _scoreService = std::make_unique<ScoreService>(_hookManager);
    _cheatService = std::make_unique<CheatService>(_hookManager);

    // These need the app to be partly initialized to properly hook
    _renderingService = std::make_shared<RenderingService>(_hookManager, _options.renderParams);
    _renderingService->SetFullscreenFlag(false);
    _inputService = std::make_shared<InputService>(_hookManager);
    _hookManager->EnableHooks();
}

HighwayPursuitServer::~HighwayPursuitServer()
{
    // Try to shutdown game, release semaphore to ensure the game sees the notification
    try
    {
        _updateService->NotifyShutdown();
        ReleaseSemaphore(_lockUpdatePool, 1, nullptr);
    }
    catch (std::exception e)
    {
        HPLogger::LogException(e);
    }

    // Release all hooks/semaphores
    _hookManager->Release();
    CloseHandle(_lockUpdatePool);
    CloseHandle(_lockServerPool);
}

void HighwayPursuitServer::Run()
{
    try
    {
        // Wait for game to be initialized
        WaitGameUpdate();
        // Enable custom qpc
        _updateService->EnableCustomTime();
        SkipIntro();

        // Get server info now that the D3D device is initialized
        BufferFormat buffer = _renderingService->GetBufferFormat();
        ServerInfo serverInfo(
            buffer.height,
            buffer.width,
            buffer.channels,
            _inputService->GetInputCount()
        );

        // Setup the communication
        _communicationManager->Connect(serverInfo);

        // For performance metrics
        _startTick = GetTickCount64();

        // Main request/response loop
        while (!_serverTerminated)
        {
            auto handler = [this](InstructionCode code) { HandleInstruction(code); };
            _communicationManager->ExecuteOnInstruction(handler);
        }
    }
    // Exception handling for the server thread
    catch (HighwayPursuitException e)
    {
        HPLogger::LogException(e);
        _communicationManager->WriteException(e);
    }
    catch (std::exception e)
    {
        HPLogger::LogException(e);
        _communicationManager->WriteException(HighwayPursuitException(ErrorCode::NATIVE_ERROR));
    }
}

void HighwayPursuitServer::WaitGameUpdate()
{
    _updateService->UpdateTime();
    ReleaseSemaphore(_lockUpdatePool, 1, nullptr);
    DWORD error = WaitForSingleObject(_lockServerPool, GAME_TIMEOUT);
    if (error)
    {
        throw HighwayPursuitException(ErrorCode::GAME_TIMEOUT);
    }
}

void HighwayPursuitServer::SkipIntro()
{
     //Calling new game skips the intro
     //We then skip the initial zoom out, and wait 2 frames for the fade out the complete
     _episodeService->NewGame();
     _renderingService->ResetZoomLevel();
     WaitGameUpdate();
     WaitGameUpdate();
}

void HighwayPursuitServer::HandleInstruction(InstructionCode code)
{
    switch (code)
    {
    case InstructionCode::RESET_NEW_LIFE:
        Reset(false);
        break;
    case InstructionCode::RESET_NEW_GAME:
        Reset(true);
        break;
    case InstructionCode::STEP:
        // Return an error if step is called without reset (player dead)
        if ((!_firstEpisodeInitialized) || _lastStepTermination.IsDone())
        {
            _communicationManager->WriteNonFatalError(ErrorCode::ENVIRONMENT_NOT_RESET);
        }
        // Step at max speed or real time depending on the option
        if (_options.isRealTime)
        {
            // The frame execution inside step will be wrapped by ExecuteForOneFrame
            Step([this](std::function<void()> action) { ExecuteForOneFrame(action); });
        }
        else
        {
            Step();
        }
        break;
    case InstructionCode::CLOSE:
        // Notify end of loop
        _serverTerminated = true;
        break;
    default:
        break;
    }
}

void HighwayPursuitServer::Reset(bool startNewGame)
{
    // Force new game the first time
    if (!_firstEpisodeInitialized)
    {
        // New game is called on the first episode because it fully resets the game state
        _episodeService->NewGame();
        // Initialize the metrics
        _currentInfo = Info(0.0f, ComputeMemoryUsage(), static_cast<float>(_cumulatedServerTicks), static_cast<float>(_cumulatedGameTicks));
        _firstEpisodeInitialized = true;
    }
    // Start a new game
    else if (startNewGame)
    {
        _episodeService->NewGame();
    }
    // The game respawns the player (new life) naturally unless the last episode wasn't terminated
    else if (!_lastStepTermination.IsDone())
    {
        _episodeService->NewLife();
    }

    // Wait for one frame for the rendering buffer to update
    WaitGameUpdate();

    // Update step variables
    _lastStepTermination = Termination(false, false);

    // Return state/info
    _renderingService->Screenshot(
        [this](void* pixelData, const BufferFormat& format)
        {
            _communicationManager->WriteObservationBuffer(pixelData, format);
        });
    _communicationManager->WriteInfoBuffer(_currentInfo);
}

// TODO: this is wrong because this doesn't take into account time between instructions
void HighwayPursuitServer::ExecuteForOneFrame(std::function<void()> action)
{
    auto start = std::chrono::high_resolution_clock::now();
    action();
    auto elapsedTicks = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start);
    double sleepTime = (TICKS_PER_FRAME - elapsedTicks.count()) / TICKS_PER_MS;
    const int sleepToleranceMillisecond = 1;

    if (sleepTime > 0)
    {
        if (sleepTime > sleepToleranceMillisecond)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(sleepTime) - sleepToleranceMillisecond));
        }

        auto duration = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start);
        while (duration.count() * TICKS_PER_MS < TICKS_PER_FRAME)
        {
            duration = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start);
        }
    }
    else
    {
        HPLogger::LogWarning("Step took longer than the intended number of frames.");
    }
}

void HighwayPursuitServer::Step(std::function<void(std::function<void()>)> frameWrapper)
{
    // Time spent server-side measurement
    ULONGLONG serverComputationStart = GetTickCount64();

    // Get action
    std::vector<Input> actions = _communicationManager->ReadActions();

    // Repeat action for _options.frameskip frames. Return early if episode ends.
    int cumulatedReward = 0;
    auto processFrame = [this, actions, &cumulatedReward]()
        {
            _inputService->SetInput(actions);

            // Apply action and get next state
            ULONGLONG gameComputationStart = GetTickCount64();
            WaitGameUpdate();
            _cumulatedGameTicks += GetTickCount64() - gameComputationStart; // measure time spent on running the game

            // Get reward
            int reward = _scoreService->PullReward();
            cumulatedReward += reward;

            // Next step
            _updateService->UpdateTime();
            _totalEllapsedFrames++;

            // Check termination (death)
            bool terminated = _episodeService->PullTerminated();
            _lastStepTermination = Termination(terminated, false);

            // Handle the metrics that are computed periodically
            if (_totalEllapsedFrames % PERIODIC_METRICS_FREQUENCY == 0)
            {
                ULONGLONG elapsedTicks = GetTickCount64() - _startTick;
                auto tps = PERIODIC_METRICS_FREQUENCY / (elapsedTicks / 1000.0f);

                float memorySize = ComputeMemoryUsage();
                _startTick = GetTickCount64();

                _currentInfo.memory = memorySize;
                _currentInfo.tps = tps;
            }
        };

    int skippedFrames = 0;
    while (skippedFrames < _options.frameskip && !_lastStepTermination.IsDone())
    {
        // Wrapper is here for the real time option
        if (frameWrapper == nullptr)
        {
            processFrame();
        }
        else
        {
            frameWrapper(processFrame);
        }
        skippedFrames++;
    }

    // Write return values
    _renderingService->Screenshot(
        [this](void* pixelData, const BufferFormat& format)
        {
            _communicationManager->WriteObservationBuffer(pixelData, format);
        });

    // Stop server-side timer
    _cumulatedServerTicks += GetTickCount64() - serverComputationStart;

    // Update metrics
    float serverTime = _cumulatedServerTicks / 1000.0f;
    float gameTime = _cumulatedGameTicks / 1000.0f;
    _currentInfo = Info(_currentInfo.tps, _currentInfo.memory, serverTime, gameTime);

    _communicationManager->WriteRewardBuffer(Reward(static_cast<float>(cumulatedReward)));
    _communicationManager->WriteInfoBuffer(_currentInfo);
    _communicationManager->WriteTerminationBuffer(_lastStepTermination);
}

float HighwayPursuitServer::ComputeMemoryUsage()
{
    float memorySize = 0.0f;

    HANDLE hProcess = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;

    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
    {
        memorySize = pmc.WorkingSetSize / (1024.0f * 1024.0f); // bytes to megabytes
    }

    return memorySize;
}