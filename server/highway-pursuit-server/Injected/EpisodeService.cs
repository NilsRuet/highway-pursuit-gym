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
        public EpisodeService(IHookManager hookManager)
        {
            this._hookManager = hookManager;
            this.RegisterFunctions();
        }

        public void NewGame()
        {
            Reset();
        }

        public void NewLife()
        {
            Respawn(0x1);
        }

        #region Hooking
        private void RegisterFunctions()
        {
            IntPtr resetPtr = new IntPtr(_hookManager.GetModuleBase().ToInt32() + MemoryAdresses.RESET_OFFSET);
            Reset = Marshal.GetDelegateForFunctionPointer<Reset_delegate>(resetPtr);

            IntPtr respawnPtr = new IntPtr(_hookManager.GetModuleBase().ToInt32() + MemoryAdresses.RESPAWN_OFFSET);
            Respawn = Marshal.GetDelegateForFunctionPointer<Respawn_delegate>(respawnPtr);
        }

        #region delegates
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void Reset_delegate();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void Respawn_delegate(byte isNotInitial);
        #endregion

        #region original function pointers
        static Reset_delegate Reset;
        static Respawn_delegate Respawn;
        #endregion
        #endregion
    }
}
