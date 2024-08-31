using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    class ScoreService
    {
        // TODO: track setScore
        //private const byte MAX_LIVES = 3;
        //private readonly IHookManager _hookManager;

        //public ScoreService(IHookManager hookManager)
        //{
        //    this._hookManager = hookManager;
        //    this.RegisterHooks();
        //}

        //#region Hooking
        //private void RegisterHooks()
        //{
        //    IntPtr getLifeCountPtr = new IntPtr(_hookManager.getModuleBase().ToInt32() + MemoryAdresses.GET_LIFE_COUNT_OFFSET);
        //    GetLifeCount = Marshal.GetDelegateForFunctionPointer<GetLifeCount_delegate>(getLifeCountPtr);
        //    _hookManager.RegisterHook(getLifeCountPtr, new GetLifeCount_delegate(GetLifeCount_Hook));
        //}

        //#region hooks
        //byte GetLifeCount_Hook()
        //{
        //    return MAX_LIVES;
        //}
        //#endregion

        //#region delegates
        //[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        //[return: MarshalAs(UnmanagedType.I1)]
        //delegate byte GetLifeCount_delegate();
        //#endregion

        //#region original function pointers
        //static GetLifeCount_delegate GetLifeCount;
        //#endregion
        //#endregion
    }
}
