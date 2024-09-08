using HighwayPursuitServer.Data;
using System;
using System.Collections.Generic;
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
        private readonly string _serverMutexName;
        private readonly string _clientMutexName;
        private readonly string _serverInfoMemoryName;
        private readonly string _instructionMemoryName;
        private readonly string _observationMemoryName;
        private readonly string _infoMemoryName;
        private readonly string _rewardMemoryName;

        private Mutex _serverMutex;
        private Mutex _clientMutex;

        private MemoryMappedViewAccessor _serverInfoSM;
        private MemoryMappedViewAccessor _instructionSM;
        private MemoryMappedViewAccessor _observationSM;
        private MemoryMappedViewAccessor _infoSM;
        private MemoryMappedViewAccessor _rewardSM;

        private List<IDisposable> _disposableResources = new List<IDisposable>();

        public CommunicationManager(string serverMutexName, string clientMutexName, string serverInfoMemoryName, string instructionMemoryName, string observationMemoryName, string infoMemoryName, string rewardMemoryName)
        {
            _serverMutexName = serverMutexName;
            _clientMutexName = clientMutexName;
            _serverInfoMemoryName = serverInfoMemoryName;
            _instructionMemoryName = instructionMemoryName;
            _observationMemoryName = observationMemoryName;
            _infoMemoryName = infoMemoryName;
            _rewardMemoryName = rewardMemoryName;
        }

        public void Connect()
        {
            // Open synchronization mutex
            _serverMutex = Mutex.OpenExisting(_serverMutexName);
            _clientMutex = Mutex.OpenExisting(_clientMutexName);

            // Wait for the client to be ready
            _serverMutex.WaitOne();

            // Open the server info memory map
            _serverInfoSM = ConnectToSharedMemory(_serverInfoMemoryName);

            var serverInfo = new ServerInfo(640, 480, 3, 8); // TODO: get the actual values
            CopyStructToSharedMemory(serverInfo, _serverInfoSM);

            _clientMutex.ReleaseMutex();

            // Now the client should have allocated all shared memory sections
            // Crate the accessors
            _serverMutex.WaitOne();

            _instructionSM = ConnectToSharedMemory(_instructionMemoryName);
            _observationSM = ConnectToSharedMemory(_observationMemoryName);
            _infoSM = ConnectToSharedMemory(_infoMemoryName);
            _rewardSM = ConnectToSharedMemory(_rewardMemoryName);

            // Give control back to the client
            _clientMutex.ReleaseMutex();
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

        private static void CopyStructToSharedMemory<T>(T data, MemoryMappedViewAccessor accessor) where T : struct
        {
            byte[] buffer = StructToBytes(data);
            accessor.WriteArray(0, buffer, 0, buffer.Length);
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
