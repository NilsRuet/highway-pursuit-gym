#include "pch.h"
#include "HighwayPursuitServer.hpp"

HighwayPursuitServer::HighwayPursuitServer(const Data::ServerParams& options)
    : TICKS_PER_FRAME(static_cast<float>(std::chrono::high_resolution_clock::period::num) / std::chrono::high_resolution_clock::period::den / FPS),
    TICKS_PER_MS(static_cast<float>(std::chrono::high_resolution_clock::period::num) / std::chrono::high_resolution_clock::period::den / 1000.0f),
    _options(options),
    _communicationManager(std::make_unique<CommunicationManager>()),
    _hookManager(std::make_unique<HookManager>()),
    _episodeService(std::make_unique<EpisodeService>()),
    _updateService(std::make_unique<UpdateService>()),
    _inputService(std::make_unique<InputService>()),
    _renderingService(std::make_unique<RenderingService>()),
    _scoreService(std::make_unique<ScoreService>()),
    _cheatService(std::make_unique<CheatService>()),
    _lockUpdatePool(),
    _lockServerPool(),
    _firstEpisodeInitialized(false),
    _serverTerminated(false),
    _totalEllapsedFrames(0),
    _lastStepTermination(false, false),
    _currentInfo(Data::Info(0.0f, 0.0f)),
    _startTick(0)
{
    // TODO: initialization
}