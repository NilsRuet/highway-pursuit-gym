using HighwayPursuitServer.Server;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    class InputService
    {
        private readonly IHookManager _hookManager;

        public InputService(IHookManager hookManager)
        {
            this._hookManager = hookManager;
            this.RegisterHooks();
        }

        #region Hooking
        private void RegisterHooks()
        {
            IntPtr getDeviceStatePtr = new IntPtr(_hookManager.GetDINPUTBase().ToInt32() + MemoryAdresses.GET_DEVICE_STATE_OFFSET);
            GetDeviceState = Marshal.GetDelegateForFunctionPointer<GetDeviceState_delegate>(getDeviceStatePtr);
            _hookManager.RegisterHook(getDeviceStatePtr, new GetDeviceState_delegate(GetDeviceState_Hook));
        }

        #region hooks
        uint GetDeviceState_Hook(IntPtr pInput, uint deviceSize, IntPtr pDeviceStateArray)
        {
            uint res = GetDeviceState(pInput, deviceSize, pDeviceStateArray);
            // Don't do anything if device acquisition failed
            if (res != 0) return res;

            // Edit the keyboard state
            byte[] modifiedState = new byte[deviceSize];
            Marshal.Copy(pDeviceStateArray, modifiedState, 0, (int)deviceSize);
            // TODO: edit keyboard state based on actions!
            Marshal.Copy(modifiedState, 0, pDeviceStateArray, (int)deviceSize);
            return res;
        }
        #endregion

        #region delegates
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U4)]
        delegate uint GetDeviceState_delegate(IntPtr pInput, uint cbData, IntPtr lpvData);
        #endregion

        #region original function pointers
        static GetDeviceState_delegate GetDeviceState;
        #endregion
        #endregion
    }
}
