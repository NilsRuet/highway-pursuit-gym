#pragma once
#include "Data/ServerTypes.hpp"

using namespace Data;

class CommunicationManager
{
public:
    static constexpr uint32_t CLIENT_TIMEOUT = 15000; // Timeout in ms

    CommunicationManager(const ServerParams& args);

    void Connect(const ServerInfo& serverInfo);
    void ExecuteOnInstruction(std::function<void(InstructionCode)> handler);
    std::vector<Input> ReadActions();
    void WriteObservationBuffer(void* buffer, const BufferFormat& format);
    void WriteInfoBuffer(const Info& info);
    void WriteRewardBuffer(const Reward& reward);
    void WriteTerminationBuffer(const Termination& termination);
    void WriteOK();
    void WriteNonFatalError(const ErrorCode& code);
    void WriteException(const HighwayPursuitException& exception);
    void Dispose();

private:
    void SyncOnClientQuery(void (*action)());
    void* ConnectToSharedMemory(const std::string& name);

    ServerParams _args;
    ServerInfo _serverInfo;

    HANDLE _lockServerPool;
    HANDLE _lockClientPool;

    void* _returnCodeSM;
    void* _serverInfoSM;
    void* _instructionSM;
    void* _observationSM;
    void* _infoSM;
    void* _rewardSM;
    void* _actionSM;
    void* _terminationSM;

    bool _cleanLastErrorOnNextRequest;
    std::vector<std::shared_ptr<void>> _disposableResources;
};

