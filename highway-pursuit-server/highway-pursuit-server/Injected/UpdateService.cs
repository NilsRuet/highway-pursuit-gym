using EasyHook;
using HighwayPursuitServer.Server;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    class UpdateService
    {
        private readonly IHookManager _hookManager;
        private readonly float _FPS; // Emulated frames per second
        private readonly long _performanceCounterFrequency; // Emulated performance counter frequency
        private readonly long _counterTicksPerFrame; // Number of performance counter ticks between each frame
        private long _performanceCount; // Most recently provided performance counter value
        private readonly Semaphore _lockServerPool;
        private readonly Semaphore _lockUpdatePool;
        private readonly bool _isRealTime; // Disable/enable performance counter hooks

        public UpdateService(IHookManager hookManager, bool isRealTime, Semaphore lockServerPool, Semaphore lockUpdatePool, float FPS, long performanceCounterFrequency)
        {
            this._hookManager = hookManager;
            this._isRealTime = isRealTime;
            this._lockServerPool = lockServerPool;
            this._lockUpdatePool = lockUpdatePool;
            this._FPS = FPS;
            this._performanceCounterFrequency = performanceCounterFrequency;
            this._performanceCount = 0;
            this._counterTicksPerFrame = (long)Math.Ceiling(_performanceCounterFrequency / _FPS);
            this.RegisterHooks();
        }

        public void UpdateTime()
        {
            if (!_isRealTime)
            {
                _performanceCount += _counterTicksPerFrame;
            }
        }

        #region Hooking
        private void RegisterHooks()
        {
            if (!_isRealTime)
            {
                // Performance counter functions
                _hookManager.RegisterHook(
                    LocalHook.GetProcAddress("kernel32.dll", "QueryPerformanceCounter"),
                    new QueryPerformanceCounter_delegate(QueryPerformanceCounter_Hook));

                _hookManager.RegisterHook(
                    LocalHook.GetProcAddress("kernel32.dll", "QueryPerformanceFrequency"),
                    new QueryPerformanceFrequency_delegate(QueryPerformanceFrequency_Hook));
            }

            // Update function
            IntPtr updatePtr = new IntPtr(_hookManager.GetModuleBase().ToInt32() + MemoryAdresses.UPDATE_OFFSET);
            Update = Marshal.GetDelegateForFunctionPointer<Update_delegate>(updatePtr);
            _hookManager.RegisterHook(updatePtr, new Update_delegate(Update_Hook));
        }

        #region hooks
        bool QueryPerformanceFrequency_Hook(out long lpFrequency)
        {
            if (_isRealTime)
            {
                throw new Exception("QueryPerformanceFrequency_Hook called in real time mode.");
            }

            lpFrequency = _performanceCounterFrequency;
            return true;
        }

        bool QueryPerformanceCounter_Hook(out long lpPerformanceCount)
        {
            if (_isRealTime)
            {
                throw new Exception("QueryPerformanceFrequency_Hook called in real time mode.");
            }

            if (_performanceCount == 0) // Initialize the previous value if necessary
            {
                QueryPerformanceCounter(out _performanceCount);
            }
            // Return the emulated value
            lpPerformanceCount = _performanceCount;
            return true;
        }

        void Update_Hook()
        {
            this._lockUpdatePool.WaitOne();
            Update();
            this._lockServerPool.Release();
        }
        #endregion

        #region delegates
        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        delegate bool QueryPerformanceFrequency_delegate(out long lpFrequency);

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        delegate bool QueryPerformanceCounter_delegate(out long lpPerformanceCount);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void Update_delegate();
        #endregion

        #region original function pointers
        // Windows functions for time
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool QueryPerformanceFrequency(out long lpFrequency);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool QueryPerformanceCounter(out long lpPerformanceCount);

        // Main game loop
        static Update_delegate Update;
        #endregion
        #endregion
    }

}
