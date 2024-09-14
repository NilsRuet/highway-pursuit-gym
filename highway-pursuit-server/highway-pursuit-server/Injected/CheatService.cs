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
    class CheatService
    {
        private readonly IHookManager _hookManager;

        public CheatService(IHookManager hookManager)
        {
            this._hookManager = hookManager;
            this.RegisterHooks();
        }

        #region Hooking
        private void RegisterHooks()
        {
            IntPtr getLivesPtr = new IntPtr(_hookManager.GetModuleBase().ToInt32() + MemoryAdresses.GET_LIVES_OFFSET);
            GetLives = Marshal.GetDelegateForFunctionPointer<GetLives_delegate>(getLivesPtr);
            _hookManager.RegisterHook(getLivesPtr, new GetLives_delegate(GetLives_Hook));
        }

        #region hooks
        byte GetLives_Hook()
        {
            return HighwayPursuitConstants.CHEATED_CONSTANT_LIVES;
        }
        #endregion

        #region delegates
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        delegate byte GetLives_delegate();
        #endregion

        #region original function pointers
        static GetLives_delegate GetLives;
        #endregion
        #endregion
    }
}
