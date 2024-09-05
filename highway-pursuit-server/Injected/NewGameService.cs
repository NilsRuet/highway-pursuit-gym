using HighwayPursuitServer.Server;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    class NewGameService
    {
        private readonly IHookManager _hookManager;
        public NewGameService(IHookManager hookManager)
        {
            this._hookManager = hookManager;
            this.RegisterHooks();
        }

        public void NewGame()
        {
            Reset();
        }

        #region Hooking
        private void RegisterHooks()
        {
            IntPtr resetPtr = new IntPtr(_hookManager.GetModuleBase().ToInt32() + MemoryAdresses.RESET_OFFSET);
            Reset = Marshal.GetDelegateForFunctionPointer<Reset_delegate>(resetPtr);
        }

        #region delegates
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void Reset_delegate();
        #endregion

        #region original function pointers
        static Reset_delegate Reset;
        #endregion
        #endregion
    }
}
