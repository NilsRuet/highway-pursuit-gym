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

            IntPtr setLivesPtr = new IntPtr(_hookManager.GetModuleBase().ToInt32() + MemoryAdresses.SET_LIVES_OFFSET);
            SetLives = Marshal.GetDelegateForFunctionPointer<SetLives_delegate>(setLivesPtr);
            _hookManager.RegisterHook(setLivesPtr, new SetLives_delegate(SetLives_Hook));
        }

        #region delegates
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void Reset_delegate();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void Respawn_delegate(byte isNotInitial);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void SetLives_delegate(byte lives);
        #endregion

        #region original function pointers
        static Reset_delegate Reset;
        static Respawn_delegate Respawn;
        static SetLives_delegate SetLives;
        #endregion

        #region hooks
        void SetLives_Hook(byte value)
        {
            SetLives(value);
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
