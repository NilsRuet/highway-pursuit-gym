#pragma once
#include "Data/ServerTypes.hpp"
#include "CommunicationManager.hpp"
#include "HookManager.hpp"
#include "Injected/CheatService.hpp"
#include "Injected/EpisodeService.hpp"
#include "Injected/InputService.hpp"
#include "Injected/RenderingService.hpp"
#include "Injected/ScoreService.hpp"
#include "Injected/UpdateService.hpp"
#include "Injected/WindowService.hpp"

using namespace Injected;

class HighwayPursuitServer
{
    public:
        static constexpr float FPS = 60.0f;
        static constexpr long PERFORMANCE_COUNTER_FREQUENCY = 1000000;
        static constexpr int GAME_TIMEOUT = 10000; // results in an error if the game fails to update
        static constexpr int METRICS_UPDATE_FREQUENCY = static_cast<int>(FPS * 30); // update info every 30s of gameplay
        static constexpr int LOG_FREQUENCY = static_cast<int>(FPS * 60); // update metrics every minute of gameplay

        HighwayPursuitServer(const Data::ServerParams& options);
        ~HighwayPursuitServer();
        void Init();
        void Run();

    private:
        float TICKS_PER_FRAME;
        float TICKS_PER_MS;
        const Data::ServerParams _options; // Game options
        std::unique_ptr<CommunicationManager> _communicationManager;
        std::shared_ptr<HookManager> _hookManager;
        std::unique_ptr<WindowService> _windowService;
        std::unique_ptr<EpisodeService> _episodeService;
        std::unique_ptr<UpdateService> _updateService;
        std::unique_ptr<ScoreService> _scoreService;
        std::unique_ptr<CheatService> _cheatService;
        // Those has to be initialized from the main thread
        std::shared_ptr<RenderingService> _renderingService;
        std::shared_ptr<InputService> _inputService;

        HANDLE _lockUpdatePool; // Update thread waits for this
        HANDLE _lockServerPool; // Server thread waits for this
        std::atomic<bool> _firstEpisodeInitialized;
        std::atomic<bool> _serverTerminated;
        uint64_t _totalEllapsedFrames;
        Data::Termination _lastStepTermination;
        Data::Info _currentInfo;
        ULONGLONG _startTick;

        void WaitGameUpdate();
        void SkipIntro();
        void HandleInstruction(InstructionCode code);
        void Reset(bool startNewGame);
        void ExecuteForOneFrame(std::function<void()> action);
        void Step(std::function<void(std::function<void()>)> frameWrapper = nullptr);
        void HandleMetrics();
        float ComputeMemoryUsage();
};

