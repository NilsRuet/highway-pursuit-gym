using HighwayPursuitServer.Data;
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
        private readonly ServerOptions _args;

        private Semaphore _lockServerPool;
        private Semaphore _lockClientPool;

        private MemoryMappedViewAccessor _serverInfoSM;
        private MemoryMappedViewAccessor _instructionSM;
        private MemoryMappedViewAccessor _observationSM;
        private MemoryMappedViewAccessor _infoSM;
        private MemoryMappedViewAccessor _rewardSM;
        private MemoryMappedViewAccessor _actionSM;
        private MemoryMappedViewAccessor _terminationSM;

        private readonly List<IDisposable> _disposableResources = new List<IDisposable>();

        public CommunicationManager(ServerOptions args)
        {
            this._args = args;
        }

        public void Connect()
        {
            // Open synchronization mutex
            _lockServerPool = Semaphore.OpenExisting(_args.serverMutexName);
            _lockClientPool = Semaphore.OpenExisting(_args.clientMutexName);

            // Wait for the client to be ready
            _lockServerPool.WaitOne();

            // Open the server info memory map
            _serverInfoSM = ConnectToSharedMemory(_args.serverInfoMemoryName);

            var serverInfo = new ServerInfo(480, 640, 4, 8); // TODO: get the actual values
            WriteStructToSharedMemory(serverInfo, _serverInfoSM);

            _lockClientPool.Release();

            // Now the client should have allocated all shared memory sections
            // Crate the accessors
            _lockServerPool.WaitOne();

            _instructionSM = ConnectToSharedMemory(_args.instructionMemoryName);
            _observationSM = ConnectToSharedMemory(_args.observationMemoryName);
            _infoSM = ConnectToSharedMemory(_args.infoMemoryName);
            _rewardSM = ConnectToSharedMemory(_args.rewardMemoryName);
            _actionSM = ConnectToSharedMemory(_args.actionMemoryName);
            _terminationSM = ConnectToSharedMemory(_args.terminationMemoryName);

            // Give control back to the client
            _lockClientPool.Release();
        }

        public void ExecuteOnInstruction(Action<InstructionCode> instructionHandler)
        {
            // Wait for the client
            // TODO: this really looks like it need a success/fail mechanism to avoid querying a failing server
            _lockServerPool.WaitOne();
            try
            {
                Instruction instruction = ReadStructFromSharedMemory<Instruction>(_instructionSM);
                instructionHandler(instruction.code);
            }
            finally
            {
                // Answer to the client
                _lockClientPool.Release();
            }
        }

        public List<Input> ReadActions()
        {
            const int actionCount = 8; // TODO: get from somewhere else
            List<Input> actions = new List<Input>();

            // Convert each non-zero byte to the corresponding action
            byte[] actionsTaken = new byte[actionCount];
            _actionSM.ReadArray(0, actionsTaken, 0, actionCount);
            for(int actionIndex = 0; actionIndex < actionCount; actionIndex++)
            {
                if(actionsTaken[actionIndex] != 0)
                {
                    actions.Add(InputUtils.IndexToInput(actionIndex));
                }
            }

            return actions;
        }

        public void WriteObservationBuffer(IntPtr buffer, D3DFORMAT format)
        {
            const int pixelCount = 640 * 480; // TODO dynamically compute pixel buffer size
            bool success = false;
            try
            {
                _observationSM.SafeMemoryMappedViewHandle.DangerousAddRef(ref success);
                IntPtr memoryPtr = _observationSM.SafeMemoryMappedViewHandle.DangerousGetHandle();
                switch (format)
                {
                    case D3DFORMAT.D3DFMT_X8R8G8B8:
                        const int channels = 4;
                        CopyMemory(memoryPtr, buffer, channels * pixelCount);
                        break;
                    default:
                        // TODO: handle error
                        break;
                }
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

        public void Dispose()
        {
            foreach(var resource in _disposableResources)
            {
                resource.Dispose();
            }
        }

        public void Log(string _)
        {
            // TODO
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
