#pragma once
#include "Data/ServerTypes.hpp"
#include <locale>
#include <codecvt>

using namespace Data;

class CommunicationManager
{
public:
    static constexpr uint32_t CLIENT_TIMEOUT = 300000; // Timeout in ms

    CommunicationManager(const ServerParams& args);
    ~CommunicationManager();

    void Connect(const ServerInfo& serverInfo);
    void ExecuteOnInstruction(std::function<void(InstructionCode)> handler);
    std::vector<Input> ReadActions();
    void WriteObservationBuffer(void* observationData, const BufferFormat& format);
    void WriteInfoBuffer(const Info& info);
    void WriteRewardBuffer(const Reward& reward);
    void WriteTerminationBuffer(const Termination& termination);
    void WriteACK();
    void WriteNonFatalError(const ErrorCode& code);
    void WriteException(const HighwayPursuitException& exception);

private:
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
    std::vector<HANDLE> _fileMappings;
    std::vector<LPVOID> _mapViews;

    void SyncOnClientQuery(std::function<void()> onQuery);
    LPVOID ConnectToSharedMemory(const std::string& name, DWORD size);
    
    template <typename T>
    static void WriteToBuffer(const T& data, void* pBuffer)
    {
        if (pBuffer == nullptr) 
        {
            throw std::runtime_error("Invalid buffer in WriteToBuffer");
        }
        T* pDest = reinterpret_cast<T*>(pBuffer);
        *pDest = data;
    }

    template <typename T>
    static T ReadFromBuffer(void* pBuffer)
    {
        if (pBuffer == nullptr)
        {
            throw std::runtime_error("Invalid buffer in ReadFromBuffer");
        }
        T* pSrc = reinterpret_cast<T*>(pBuffer);
        T data = *pSrc;
        return data;
    }

    template <typename T>
    static void ReadArrayFromBuffer(std::vector<T>& returnBuffer, void* pBuffer, size_t count)
    {
        if (pBuffer == nullptr)
        {
            throw std::runtime_error("Invalid buffer in ReadArrayFromBuffer");
        }
        returnBuffer.resize(count);
        std::memcpy(returnBuffer.data(), pBuffer, count * sizeof(T));
    }
};

