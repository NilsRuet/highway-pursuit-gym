#include "pch.h"
#include "CommunicationManager.hpp"

CommunicationManager::CommunicationManager(const ServerParams& args)
    : _args(args),
    _cleanLastErrorOnNextRequest(false),
    _serverInfo(ServerInfo(0, 0, 0, 0)),
    _lockServerPool(nullptr),
    _lockClientPool(nullptr),
    _returnCodeSM(nullptr),
    _serverInfoSM(nullptr),
    _instructionSM(nullptr),
    _observationSM(nullptr),
    _infoSM(nullptr),
    _rewardSM(nullptr),
    _actionSM(nullptr),
    _terminationSM(nullptr),
    _fileMappings({}),
    _mapViews({})
{
   
}

CommunicationManager::~CommunicationManager()
{
    for (auto handle: _mapViews)
    {
        UnmapViewOfFile(handle);
    }

    for (auto pBuffer : _fileMappings)
    {
        UnmapViewOfFile(pBuffer);
    }

    if (_lockServerPool != nullptr)
    {
        CloseHandle(_lockServerPool);
    }

    if (_lockClientPool != nullptr)
    {
        CloseHandle(_lockClientPool);
    }
}

void CommunicationManager::Connect(const ServerInfo& serverInfo)
{
    // Update current server info
    _serverInfo = serverInfo;

    // Open synchronization semaphores mutex
    _lockServerPool = OpenSemaphoreA(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, false, _args.serverMutexName.c_str());
    if (_lockServerPool == nullptr)
    {
        throw std::runtime_error("Couldn't get semaphore, error " + std::to_string(GetLastError()));
    }

    _lockClientPool = OpenSemaphoreA(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, true, _args.clientMutexName.c_str());
    if (_lockClientPool == nullptr)
    {
        throw std::runtime_error("Couldn't get semaphore, error " + std::to_string(GetLastError()));
    }

    SyncOnClientQuery([this]()
        {
            _returnCodeSM = ConnectToSharedMemory(_args.returnCodeMemoryName, sizeof(ServerInfo));
            _serverInfoSM = ConnectToSharedMemory(_args.serverInfoMemoryName, sizeof(ReturnCode));

            WriteACK();
            WriteToBuffer(_serverInfo, _serverInfoSM);
        }
    );

    SyncOnClientQuery([this]()
        {
            size_t actionSize = _serverInfo.actionCount;
            size_t observationSize = _serverInfo.obsWidth* _serverInfo.obsHeight* _serverInfo.obsChannels;

            _instructionSM = ConnectToSharedMemory(_args.instructionMemoryName, sizeof(Instruction));
            _observationSM = ConnectToSharedMemory(_args.observationMemoryName, observationSize);
            _infoSM = ConnectToSharedMemory(_args.infoMemoryName, sizeof(Info));
            _rewardSM = ConnectToSharedMemory(_args.rewardMemoryName, sizeof(Instruction));
            _actionSM = ConnectToSharedMemory(_args.actionMemoryName, actionSize);
            _terminationSM = ConnectToSharedMemory(_args.terminationMemoryName, sizeof(Termination));
        }
    );
}

// ExecuteOnInstruction method
void CommunicationManager::ExecuteOnInstruction(std::function<void(InstructionCode)> handler)
{
    SyncOnClientQuery([this, handler]()
    {
        Instruction instruction = ReadFromBuffer<Instruction>(_instructionSM);
        handler(instruction.code);
    });
}

// ReadActions method
std::vector<Input> CommunicationManager::ReadActions()
{
    uint32_t actionCount = _serverInfo.actionCount;
    std::vector<Input> actions;

    // Convert each non-zero byte to the corresponding action
    std::vector<uint8_t> actionsTaken(actionCount);
    ReadArrayFromBuffer<uint8_t>(actionsTaken, _actionSM, actionCount);
    for (uint32_t actionIndex = 0; actionIndex < actionCount; ++actionIndex)
    {
        if (actionsTaken[actionIndex] != 0)
        {
            actions.push_back(InputUtils::IndexToInput(actionIndex));
        }
    }
    return actions;
}

void CommunicationManager::WriteObservationBuffer(void* observationData, const BufferFormat& format)
{
    // This is faster than memcpy
    RtlCopyMemory(_observationSM, observationData, format.Size());
}

void CommunicationManager::WriteInfoBuffer(const Info& info)
{
    WriteToBuffer(info, _infoSM);
}

void CommunicationManager::WriteRewardBuffer(const Reward& reward)
{
    WriteToBuffer(reward, _rewardSM);
}

void CommunicationManager::WriteTerminationBuffer(const Termination& termination)
{
    WriteToBuffer(termination, _terminationSM);
}

void CommunicationManager::WriteACK()
{
    WriteToBuffer(ErrorCode::ACKNOWLEDGED, _returnCodeSM);
}

void CommunicationManager::WriteNonFatalError(const ErrorCode& code)
{
    WriteToBuffer(code, _returnCodeSM);
}

void CommunicationManager::WriteException(const HighwayPursuitException& exception)
{
    WriteToBuffer(exception.code, _returnCodeSM);
}

void CommunicationManager::SyncOnClientQuery(std::function<void()> onQuery)
{
    DWORD error = WaitForSingleObject(_lockServerPool, CLIENT_TIMEOUT);
    if (!error)
    {
        try
        {
            if (_cleanLastErrorOnNextRequest)
            {
                WriteACK();
                _cleanLastErrorOnNextRequest = false;
            }
            onQuery();
        }
        // We need to write the exception before releasing so the client doesn't use invaldi data
        catch (HighwayPursuitException e)
        {
            // Answer to the client
            WriteException(e);
            ReleaseSemaphore(_lockClientPool, 1, nullptr);
            throw;
        }
        catch (...)
        {
            // Answer to the client
            WriteException(HighwayPursuitException(ErrorCode::NATIVE_ERROR));
            ReleaseSemaphore(_lockClientPool, 1, nullptr);
            throw;
        }

        // Answer to the client
        bool res = ReleaseSemaphore(_lockClientPool, 1, nullptr);
        if (!res)
        {
            throw std::runtime_error("Failed to release semaphore. Error "+std::to_string(GetLastError()));
        }
    }
    else
    {
        throw HighwayPursuitException(ErrorCode::CLIENT_TIMEOUT);
    }
}

LPVOID CommunicationManager::ConnectToSharedMemory(const std::string& name, DWORD size)
{
    HANDLE hMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS, // Request read/write access
        FALSE,               // Do not inherit the handle
        name.c_str() // Name of the shared memory
    );

    if (hMapFile == nullptr)
    {
        throw std::runtime_error("Couldn't open FileMapping, error " + std::to_string(GetLastError()));
    }

    LPVOID pBuf = MapViewOfFile(
        hMapFile,            // Handle to the map object
        FILE_MAP_ALL_ACCESS, // Read/write access
        0,
        0,
        size                 // Size of the mapping
    );

    if (pBuf == nullptr)
    {
        throw std::runtime_error("Couldn't get MapView, error " + std::to_string(GetLastError()));
    }

    _mapViews.push_back(pBuf);
    _fileMappings.push_back(hMapFile);

    return pBuf;
}