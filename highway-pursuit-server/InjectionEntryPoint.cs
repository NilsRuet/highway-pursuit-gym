using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;
using Direct3D8;

namespace HighwayPursuitServer
{
    public class InjectionEntryPoint : EasyHook.IEntryPoint
    {
        ServerInterface _server = null;
        Queue<string> _messageQueue = new Queue<string>();

        public InjectionEntryPoint(EasyHook.RemoteHooking.IContext context, string channelName)
        {
            _server = EasyHook.RemoteHooking.IpcConnectClient<ServerInterface>(channelName);
            _server.Ping();
        }

        public void Run(EasyHook.RemoteHooking.IContext context, string channelName)
        {
            // Module base for custom functions
            IntPtr moduleBase = Process.GetCurrentProcess().MainModule.BaseAddress;

            // Injection is now complete and the server interface is connected
            _server.IsInstalled(EasyHook.RemoteHooking.GetCurrentProcessId());

            // Install hooks
            // QPC https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter
            var qpcHook = EasyHook.LocalHook.Create(
                EasyHook.LocalHook.GetProcAddress("kernel32.dll", "QueryPerformanceCounter"),
                new QueryPerformanceCounter_delegate(QueryPerformanceCounter_Hook),
                this);

            var qpfHook = EasyHook.LocalHook.Create(
               EasyHook.LocalHook.GetProcAddress("kernel32.dll", "QueryPerformanceFrequency"),
               new QueryPerformanceFrequency_delegate(QueryPerformanceFrequency_Hook),
               this);

            // Find/Hook the original get life count function
            var getLifeCountOffset = 0x15C70;
            IntPtr getLifeCountPtr = new IntPtr(moduleBase.ToInt32() + getLifeCountOffset);
            GetLifeCount = Marshal.GetDelegateForFunctionPointer<GetLifeCount_delegate>(getLifeCountPtr);

            var getLifeCountHook = EasyHook.LocalHook.Create(
                getLifeCountPtr,                     // Address of the function to hook
                new GetLifeCount_delegate(GetLifeCount_Hook),  // Delegate to the hooked function
                null                                   // Callback parameter (not used here)
            );


            // Find/Hook the original get life count function
            var setLifeCountOffset = 0x15C50;
            IntPtr setLifeCountPtr = new IntPtr(moduleBase.ToInt32() + setLifeCountOffset);
            SetLifeCount = Marshal.GetDelegateForFunctionPointer<SetLifeCount_delegate>(setLifeCountPtr);

            var setLifeCountHook = EasyHook.LocalHook.Create(
                setLifeCountPtr,                     // Address of the function to hook
                new SetLifeCount_delegate(SetLifeCount_Hook),  // Delegate to the hooked function
                null                                   // Callback parameter (not used here)
            );


            // Find/Hook the original update
            var updateOffset = 0x29420;
            IntPtr updatePtr = new IntPtr(moduleBase.ToInt32() + updateOffset);
            Update = Marshal.GetDelegateForFunctionPointer<Update_delegate>(updatePtr);

            var updateHook = EasyHook.LocalHook.Create(
                updatePtr,
                new Update_delegate(Update_Hook),
                null
            );


            // Activate hooks on all threads except the current thread
            qpcHook.ThreadACL.SetExclusiveACL(new int[] { 0 });
            qpfHook.ThreadACL.SetExclusiveACL(new int[] { 0 });
            getLifeCountHook.ThreadACL.SetExclusiveACL(new int[] { 0 });
            setLifeCountHook.ThreadACL.SetExclusiveACL(new int[] { 0 });
            updateHook.ThreadACL.SetExclusiveACL(new int[] { 0 });

            _server.ReportMessage("Hooks installed");

            // Wake up the process (required if using RemoteHooking.CreateAndInject)
            EasyHook.RemoteHooking.WakeUpProcess();

            try
            {
                // Loop until FileMonitor closes (i.e. IPC fails)
                while (true)
                {
                    System.Threading.Thread.Sleep(500);

                    string[] queued = null;

                    lock (_messageQueue)
                    {
                        queued = _messageQueue.ToArray();
                        _messageQueue.Clear();
                    }

                    // Send newly monitored file accesses to FileMonitor
                    if (queued != null && queued.Length > 0)
                    {
                        _server.ReportMessages(queued);
                    }
                    else
                    {
                        _server.Ping();
                    }
                }
            }
            catch
            {
                // Ping() or ReportMessages() will raise an exception if host is unreachable
            }

            // Remove hooks
            qpcHook.Dispose();
            qpfHook.Dispose();
            getLifeCountHook.Dispose();
            setLifeCountHook.Dispose();
            updateHook.Dispose();

            // Finalise cleanup of hooks
            EasyHook.LocalHook.Release();
        }

        #region performance counter 
        static long QPCValue = 0;
        const float FPS = 60.0f;
        const long QPFrequency = 1000000;
        const long QPCFrame = (int)(QPFrequency / FPS);
        #endregion

        #region QueryPerformanceFrequency Hook

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        delegate bool QueryPerformanceFrequency_delegate(
          out long lpFrequency
        );

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool QueryPerformanceFrequency(out long lpFrequency);

        static long actualPCFrequency;
        bool QueryPerformanceFrequency_Hook(
            out long lpFrequency)
        {
            // Call original first
            QueryPerformanceFrequency(out actualPCFrequency);

            //return result;
            lpFrequency = QPFrequency;
            return true;
        }

        #endregion


        #region QueryPerformanceCounter Hook

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        delegate bool QueryPerformanceCounter_delegate(
          out long lpPerformanceCount
        );

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool QueryPerformanceCounter(out long lpPerformanceCount);


        static long actualQPC = 0;

        bool QueryPerformanceCounter_Hook(
            out long lpPerformanceCount)
        {
            QueryPerformanceCounter(out var originalValue);
            actualQPC = originalValue;
            // Call original first
            if (QPCValue == 0)
            {

                QPCValue = originalValue;
            }

            lpPerformanceCount = QPCValue;
            return true;
        }
        #endregion

        #region GetLifeCount Hook

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        delegate byte GetLifeCount_delegate();

        // Original function, assigned later
        static GetLifeCount_delegate GetLifeCount;

        byte GetLifeCount_Hook()
        {
            // Call original first
            var lifeCount = GetLifeCount();lifeCount = 3;
            return lifeCount;
        }

        #endregion
        #region SetLifeCount Hook

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void SetLifeCount_delegate(int count);

        // Original function, assigned later
        static SetLifeCount_delegate SetLifeCount;

        #region d3d8

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U8)]
        delegate long CreateImageSurface_delegate(IntPtr pDevice, uint width, uint height, uint format, ref uint pSurface);

        #endregion

        void SetLifeCount_Hook(int count)
        {
            // Call original first
            SetLifeCount(count);

            if (count == 2)
            {
                IntPtr moduleBase = Process.GetCurrentProcess().MainModule.BaseAddress;
                IntPtr d3d8Base = new IntPtr(0);
                foreach (ProcessModule module in Process.GetCurrentProcess().Modules)
                {
                    if (module.ModuleName == "d3d8.dll")
                    {
                        d3d8Base = module.BaseAddress;
                        break;
                    }
                }

                if (d3d8Base.ToInt32() == 0) return;

                var createImageSurfaceOffset = 0x2B6F0;
                IntPtr createImageSurfacePtr = new IntPtr(d3d8Base.ToInt32() + createImageSurfaceOffset);
                uint pSurface = 0;

                var deviceOffset = 0x960CC;
                IntPtr pDevice = new IntPtr(Marshal.ReadInt32(new IntPtr(moduleBase.ToInt32() + deviceOffset)));

                var function = Marshal.GetDelegateForFunctionPointer<CreateImageSurface_delegate>(createImageSurfacePtr);
                function(pDevice, 10u, 10u, (uint)D3DFORMAT.D3DFMT_A8R8G8B8, ref pSurface);

                lock (this._messageQueue)
                {
                    if (this._messageQueue.Count < 1000)
                    {
                        // Add message to send to FileMonitor
                        this._messageQueue.Enqueue(
                            $" Called create surface {pSurface:x}");
                    }
                }
            }

            try
            {
                lock (this._messageQueue)
                {
                    if (this._messageQueue.Count < 1000)
                    {
                        var process = EasyHook.RemoteHooking.GetCurrentProcessId();
                        var threadId = EasyHook.RemoteHooking.GetCurrentThreadId();
                        // Add message to send to FileMonitor
                        this._messageQueue.Enqueue(
                            $"[{process}:{threadId}]: SetLifeCount {count}");
                    }
                }
            }
            catch
            {
                // swallow exceptions so that any issues caused by this code do not crash target process
            }
        }

        #endregion
        #region Update Hook

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void Update_delegate();

        // Original function, assigned later
        static Update_delegate Update;
        static long t1 = 0;
        static int callCount = 0;

        void Update_Hook()
        {
            Update();
            QPCValue += QPCFrame;
            callCount += 1;

            if (t1 == 0)
            {
                QueryPerformanceCounter(out _);
                t1 = actualQPC;
            }

            var callCountTarget = 1000;
            if (callCount % callCountTarget == 0)
            {
                QueryPerformanceCounter(out var _);
                var t2 = actualQPC;
                QueryPerformanceFrequency(out var _);
                var frequency = actualPCFrequency;

                var time = (t2 - t1) / (float)frequency;
                callCount = 0;
                var rate = callCountTarget / time;
                try
                {
                    lock (this._messageQueue)
                    {
                        if (this._messageQueue.Count < 1000)
                        {
                            var process = EasyHook.RemoteHooking.GetCurrentProcessId();
                            var threadId = EasyHook.RemoteHooking.GetCurrentThreadId();
                            // Add message to send to FileMonitor
                            this._messageQueue.Enqueue(
                                $"[{process}:{threadId}]: {rate} updates/s");
                        }
                    }
                }
                catch
                {
                    // swallow exceptions so that any issues caused by this code do not crash target process
                }
                t1 = t2;
            }
        }

        #endregion
    }
}
