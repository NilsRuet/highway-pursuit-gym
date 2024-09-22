#include "pch.h"
#include "HighwayPursuitServer.hpp"

HighwayPursuitServer::HighwayPursuitServer(const Data::ServerParams& options)
    : _options(options),
    _firstEpisodeInitialized(false),
    _serverTerminated(false),
    _totalEllapsedFrames(0),
    _lastStepTermination(false, false),
    _currentInfo(Data::Info(0.0f, 0.0f)),
    _startTick(0)
{
    // num/den is the period in seconds, den/num is therefore the frequency in hertz
    auto ticks_per_s = (static_cast<float>(std::chrono::high_resolution_clock::period::den) / std::chrono::high_resolution_clock::period::num);
    TICKS_PER_FRAME = ticks_per_s / FPS;
    TICKS_PER_MS = ticks_per_s / 1000.0f;

    // init communication manager
    _communicationManager = std::make_unique<CommunicationManager>(options);

    // init update semaphores
    _lockUpdatePool = CreateSemaphore(nullptr, 0, 1, nullptr);
    _lockServerPool = CreateSemaphore(nullptr, 0, 1, nullptr);

    // init services
    _hookManager = std::make_shared<HookManager>();

    LARGE_INTEGER frequency;
    frequency.QuadPart = PERFORMANCE_COUNTER_FREQUENCY;
    _episodeService = std::make_unique<EpisodeService>(_hookManager);
    _updateService = std::make_unique<UpdateService>(_hookManager, options.isRealTime, _lockServerPool, _lockUpdatePool, FPS, frequency);
    _inputService = std::make_unique<InputService>(_hookManager);
    _renderingService = std::make_unique<RenderingService>(_hookManager, options.renderParams);
    _scoreService = std::make_unique<ScoreService>(_hookManager);
    _cheatService = std::make_unique<CheatService>(_hookManager);

    // Enable hooks
    _hookManager->EnableHooks();
    _renderingService->SetFullscreenFlag(false);
}

HighwayPursuitServer::~HighwayPursuitServer()
{
    _updateService->DisableSemaphores();
    ReleaseSemaphore(_lockUpdatePool, 1, nullptr);
    CloseHandle(_lockUpdatePool);
    CloseHandle(_lockServerPool);
}

void HighwayPursuitServer::Run()
{
    try
    {
        // Wait for game to be initialized
        WaitGameUpdate();
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
        _communicationManager->WriteException(e);
        HPLogger::LogException(e);
    }
    catch (std::exception e)
    {
        _communicationManager->WriteException(HighwayPursuitException(ErrorCode::NATIVE_ERROR));
        HPLogger::LogException(e);
    }
    _hookManager->Release();
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
        // ensure the updates hooks are non-blocking
        _updateService->DisableSemaphores();
        ReleaseSemaphore(_lockUpdatePool, 1, nullptr);
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
        _currentInfo = Info(0.0f, ComputeMemoryUsage());
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
    // Get action
    std::vector<Input> actions = _communicationManager->ReadActions();

    // Repeat action for _options.frameskip frames. Return early if episode ends.
    int cumulatedReward = 0;
    auto processFrame = [this, actions, &cumulatedReward]()
        {
            _inputService->SetInput(actions);
            // Apply action and get next state
            WaitGameUpdate();

            // Get reward
            int reward = _scoreService->PullReward();
            cumulatedReward += reward;

            // Next step
            _updateService->UpdateTime();
            _totalEllapsedFrames++;

            // Check termination (death)
            bool terminated = _episodeService->PullTerminated();
            _lastStepTermination = Termination(terminated, false);

            // Handle the computation of useful metrics
            HandleMetrics();
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
    _communicationManager->WriteRewardBuffer(Reward(static_cast<float>(cumulatedReward)));
    _communicationManager->WriteInfoBuffer(_currentInfo);
    _communicationManager->WriteTerminationBuffer(_lastStepTermination);
}

void HighwayPursuitServer::HandleMetrics()
{
    // Performance metrics
    if (_totalEllapsedFrames % METRICS_UPDATE_FREQUENCY == 0)
    {
        ULONGLONG elapsedTicks = GetTickCount64() - _startTick;
        auto tps = _totalEllapsedFrames / (elapsedTicks / 1000.0f);

        float memorySize = ComputeMemoryUsage();
        _currentInfo = Info(tps, memorySize);
        _startTick = GetTickCount64();
    }
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