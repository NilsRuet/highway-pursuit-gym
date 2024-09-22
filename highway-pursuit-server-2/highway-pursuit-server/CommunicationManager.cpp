#include "pch.h"
#include "CommunicationManager.hpp"

CommunicationManager::CommunicationManager(const ServerParams& args) : _args(args), _cleanLastErrorOnNextRequest(false), _serverInfo(ServerInfo(0, 0, 0, 0))
{
    // TODO
}

// Connect method
void CommunicationManager::Connect(const ServerInfo& serverInfo)
{
    // TODO: Implement the body
}

// ExecuteOnInstruction method
void CommunicationManager::ExecuteOnInstruction(std::function<void(InstructionCode)> handler)
{
    // TODO: Implement the body
}

// ReadActions method
std::vector<Input> CommunicationManager::ReadActions()
{
    // TODO: Implement the body
    return std::vector<Input>();
}

// WriteObservationBuffer method
void CommunicationManager::WriteObservationBuffer(void* buffer, const BufferFormat& format)
{
    // TODO: Implement the body
}

// WriteInfoBuffer method
void CommunicationManager::WriteInfoBuffer(const Info& info)
{
    // TODO: Implement the body
}

// WriteRewardBuffer method
void CommunicationManager::WriteRewardBuffer(const Reward& reward)
{
    // TODO: Implement the body
}

// WriteTerminationBuffer method
void CommunicationManager::WriteTerminationBuffer(const Termination& termination)
{
    // TODO: Implement the body
}

// WriteOK method
void CommunicationManager::WriteOK()
{
    // TODO: Implement the body
}

// WriteNonFatalError method
void CommunicationManager::WriteNonFatalError(const ErrorCode& code)
{
    // TODO: Implement the body
}

// WriteException method
void CommunicationManager::WriteException(const HighwayPursuitException& exception)
{
    // TODO: Implement the body
}

// Dispose method
void CommunicationManager::Dispose()
{
    // TODO: Implement the body
}

// SyncOnClientQuery method
void CommunicationManager::SyncOnClientQuery(void (*action)())
{
    // TODO: Implement the body
}

// ConnectToSharedMemory method
void* CommunicationManager::ConnectToSharedMemory(const std::string& name)
{
    // TODO: Implement the body
    return nullptr;
}