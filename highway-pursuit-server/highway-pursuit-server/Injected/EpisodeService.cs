using HighwayPursuitServer.Data;
using HighwayPursuitServer.Server;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    class EpisodeService
    {
        private readonly IHookManager _hookManager;
        private bool _terminated = false;

        public EpisodeService(IHookManager hookManager)
        {
            this._hookManager = hookManager;
            this.RegisterHooks();
        }

        public void NewGame()
        {
            Reset();
        }

        public void NewLife()
        {
            Respawn(0x1);
        }

        public void Truncate()
        {
            NewLife();
        }

        public bool PullTerminated()
        {
            var res = _terminated;
            _terminated = false;
            return res;
        }

        #region Hooking
        private void RegisterHooks()
        {
            IntPtr resetPtr = new IntPtr(_hookManager.GetModuleBase().ToInt32() + MemoryAdresses.RESET_OFFSET);
            Reset = Marshal.GetDelegateForFunctionPointer<Reset_delegate>(resetPtr);

            IntPtr respawnPtr = new IntPtr(_hookManager.GetModuleBase().ToInt32() + MemoryAdresses.RESPAWN_OFFSET);
            Respawn = Marshal.GetDelegateForFunctionPointer<Respawn_delegate>(respawnPtr);

            IntPtr setLifeCountPtr = new IntPtr(_hookManager.GetModuleBase().ToInt32() + MemoryAdresses.SET_LIFE_COUNT_OFFSET);
            SetLifeCount = Marshal.GetDelegateForFunctionPointer<SetLifeCount_delegate>(setLifeCountPtr);
            _hookManager.RegisterHook(setLifeCountPtr, new SetLifeCount_delegate(SetLifeCount_Hook));
        }

        #region delegates
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void Reset_delegate();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void Respawn_delegate(byte isNotInitial);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void SetLifeCount_delegate(byte lifeCount);
        #endregion

        #region original function pointers
        static Reset_delegate Reset;
        static Respawn_delegate Respawn;
        static SetLifeCount_delegate SetLifeCount;
        #endregion

        #region hooks
        void SetLifeCount_Hook(byte value)
        {
            SetLifeCount(value);
            // If one life was lost
            if(value == HighwayPursuitConstants.CHEATED_CONSTANT_LIVES - 1)
            {
                _terminated = true;
            }
        }
        #endregion
        #endregion
    }
}
