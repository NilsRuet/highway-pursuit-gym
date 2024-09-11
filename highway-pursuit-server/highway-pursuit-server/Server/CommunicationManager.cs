using HighwayPursuitServer.Data;
using HighwayPursuitServer.Exceptions;
using HighwayPursuitServer.Injected;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Server
{
    class CommunicationManager
    {
        const int CLIENT_TIMEOUT = 2000; // TIMEOUT in ms

        private readonly ServerOptions _args;
        private ServerInfo _serverInfo;

        private Semaphore _lockServerPool;
        private Semaphore _lockClientPool;

        private MemoryMappedViewAccessor _returnCodeSM;
        private MemoryMappedViewAccessor _serverInfoSM;
        private MemoryMappedViewAccessor _instructionSM;
        private MemoryMappedViewAccessor _observationSM;
        private MemoryMappedViewAccessor _infoSM;
        private MemoryMappedViewAccessor _rewardSM;
        private MemoryMappedViewAccessor _actionSM;
        private MemoryMappedViewAccessor _terminationSM;

        private bool _cleanLastErrorOnNextRequest = false;

        private readonly List<IDisposable> _disposableResources = new List<IDisposable>();

        public CommunicationManager(ServerOptions args)
        {
            this._args = args;
        }

        public void Connect(ServerInfo serverInfo)
        {
            // Update current server info
            _serverInfo = serverInfo;

            // Open synchronization mutex
            _lockServerPool = Semaphore.OpenExisting(_args.serverMutexName);
            _lockClientPool = Semaphore.OpenExisting(_args.clientMutexName);

            // Wait for the client to be ready and send the server info via the shared memory
            SyncOnClientQuery(() => {
                _returnCodeSM = ConnectToSharedMemory(_args.returnCodeMemoryName);
                _serverInfoSM = ConnectToSharedMemory(_args.serverInfoMemoryName);

                WriteOK();
                WriteStructToSharedMemory(_serverInfo, _serverInfoSM);
            });

            // Create the remaining memory accessors
            SyncOnClientQuery(() => {
                _instructionSM = ConnectToSharedMemory(_args.instructionMemoryName);
                _observationSM = ConnectToSharedMemory(_args.observationMemoryName);
                _infoSM = ConnectToSharedMemory(_args.infoMemoryName);
                _rewardSM = ConnectToSharedMemory(_args.rewardMemoryName);
                _actionSM = ConnectToSharedMemory(_args.actionMemoryName);
                _terminationSM = ConnectToSharedMemory(_args.terminationMemoryName);
            });
        }

        public void ExecuteOnInstruction(Action<InstructionCode> instructionHandler)
        {
            SyncOnClientQuery(() =>
            {
                Instruction instruction = ReadStructFromSharedMemory<Instruction>(_instructionSM);
                instructionHandler(instruction.code);
            });
        }

        private void SyncOnClientQuery(Action action)
        {
            bool acquired = _lockServerPool.WaitOne(CLIENT_TIMEOUT);
            if (acquired)
            {
                try
                {
                    if (_cleanLastErrorOnNextRequest)
                    {
                        WriteOK();
                        _cleanLastErrorOnNextRequest = false;
                    }

                    action();
                }
                // We need to write the exception before releasing so the client doesn't use invaldi data
                catch(HighwayPursuitException e)
                {
                    WriteException(e);
                    throw e;
                }
                catch (Exception e)
                {
                    WriteException(new HighwayPursuitException(ErrorCode.NATIVE_ERROR));
                    throw e;
                }
                finally
                {
                    // Answer to the client
                    _lockClientPool.Release();
                }
            } else
            {
                throw new HighwayPursuitException(ErrorCode.CLIENT_TIMEOUT);
            }
        }

        public List<Input> ReadActions()
        {
            List<Input> actions = new List<Input>();

            // Convert each non-zero byte to the corresponding action
            uint actionCount = _serverInfo.actionCount;
            byte[] actionsTaken = new byte[actionCount];
            _actionSM.ReadArray(0, actionsTaken, 0, (int)actionCount);
            for(int actionIndex = 0; actionIndex < actionCount; actionIndex++)
            {
                if(actionsTaken[actionIndex] != 0)
                {
                    actions.Add(InputUtils.IndexToInput(actionIndex));
                }
            }

            return actions;
        }

        public void WriteObservationBuffer(IntPtr buffer)
        {
            bool success = false;
            try
            {
                _observationSM.SafeMemoryMappedViewHandle.DangerousAddRef(ref success);
                IntPtr memoryPtr = _observationSM.SafeMemoryMappedViewHandle.DangerousGetHandle();
                var bufferSize = _serverInfo.obsWidth * _serverInfo.obsHeight * _serverInfo.obsChannels;
                CopyMemory(memoryPtr, buffer, bufferSize);
            }
            finally
            {
                if (success)
                {
                    _observationSM.SafeMemoryMappedViewHandle.DangerousRelease();
                }
            }
        }

        public void WriteInfoBuffer(Info info)
        {
            WriteStructToSharedMemory(info, _infoSM);
        }

        public void WriteRewardBuffer(Reward reward)
        {
            WriteStructToSharedMemory(reward, _rewardSM);
        }

        public void WriteTerminationBuffer(Termination termination)
        {
            WriteStructToSharedMemory(termination, _terminationSM);
        }

        private MemoryMappedViewAccessor ConnectToSharedMemory(string name)
        {
            var mmf = MemoryMappedFile.OpenExisting(name);
            _disposableResources.Add(mmf);

            var accessor = mmf.CreateViewAccessor();
            _disposableResources.Add(accessor);

            return accessor;
        }

        public void WriteOK()
        {
            // The return code is set to ACK once when the client connects to the server
            // As a way to inform the client that the server is initialized properly
            WriteStructToSharedMemory(new ReturnCode(ErrorCode.ACKNOWLEDGED), _returnCodeSM);
        }

        public void WriteNonFatalError(ErrorCode code)
        {
            WriteStructToSharedMemory(new ReturnCode(code), _returnCodeSM);
            _cleanLastErrorOnNextRequest = true;
        }

        public void WriteException(HighwayPursuitException exception)
        {
            WriteStructToSharedMemory(new ReturnCode(exception.code), _returnCodeSM);
        }

        public void Dispose()
        {
            foreach(var resource in _disposableResources)
            {
                resource.Dispose();
            }
        }

        private static void WriteStructToSharedMemory<T>(T data, MemoryMappedViewAccessor accessor) where T : struct
        {
            byte[] buffer = StructToBytes(data);
            accessor.WriteArray(0, buffer, 0, buffer.Length);
        }

        private static T ReadStructFromSharedMemory<T>(MemoryMappedViewAccessor accessor) where T : struct
        {
            accessor.Read(0, out T result);
            return result;
        }

        [DllImport("kernel32.dll", EntryPoint = "RtlMoveMemory")]
        private static extern void CopyMemory(IntPtr dest, IntPtr src, uint count);

        private static byte[] StructToBytes<T>(T obj) where T : struct
        {
            int size = Marshal.SizeOf(obj);
            byte[] arr = new byte[size];

            GCHandle handle = GCHandle.Alloc(arr, GCHandleType.Pinned);
            try
            {
                IntPtr arrPtr = handle.AddrOfPinnedObject();
                IntPtr structPtr = Marshal.AllocHGlobal(size);
                try
                {
                    Marshal.StructureToPtr(obj, structPtr, true);
                    CopyMemory(arrPtr, structPtr, (uint)size);
                }
                finally
                {
                    Marshal.FreeHGlobal(structPtr);
                }
            }
            finally
            {
                handle.Free();
            }

            return arr;
        }
    }
}
