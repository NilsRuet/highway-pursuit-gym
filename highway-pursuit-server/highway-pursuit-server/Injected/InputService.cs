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
    class InputService
    {

        private readonly Dictionary<Input, uint> _inputToOffset = new Dictionary<Input, uint>{
            {Input.Accelerate, MemoryAdresses.ACCELERATE_OFFSET},
            {Input.Brake, MemoryAdresses.BRAKE_OFFSET},
            {Input.SteerL, MemoryAdresses.STEER_L_OFFSET},
            {Input.SteerR, MemoryAdresses.STEER_R_OFFSET},
            {Input.Fire, MemoryAdresses.FIRE_OFFSET},
            {Input.Oil, MemoryAdresses.OIL_OFFSET},
            {Input.Smoke, MemoryAdresses.SMOKE_OFFSET},
            {Input.Missiles, MemoryAdresses.MISSILES_OFFSET},
        };

        private List<Input> _currentInputs = new List<Input>();
        private const byte ACTIVE_KEY = 0x80;

#if DEBUG
        private const byte MANUAL_CONTROL_KEY = 0x2A; // holding left shift enables manual keyboard control
#endif

        private readonly IHookManager _hookManager;

        public InputService(IHookManager hookManager)
        {
            this._hookManager = hookManager;
            this.RegisterHooks();
        }

        #region input management
        public int GetInputCount()
        {
            return _inputToOffset.Count;
        }
        
        public void SetInput(List<Input> inputs)
        {
            this._currentInputs = inputs;
        }

        private int InputToKeyCode(Input input)
        {
            var offset = _inputToOffset[input];
            var module = _hookManager.GetModuleBase();
            return Marshal.ReadInt32(new IntPtr(module.ToInt32() + offset));
        }
        #endregion

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

#if DEBUG
            // Don't do anything in manual control
            byte[] originalState = new byte[deviceSize];
            Marshal.Copy(pDeviceStateArray, originalState, 0, (int)deviceSize);
            if (originalState[MANUAL_CONTROL_KEY] == ACTIVE_KEY)
            {
                return res;
            }
#endif

            // Edit the keyboard state
            byte[] modifiedState = new byte[deviceSize];

            foreach (Input input in _currentInputs)
            {
                var keycode = InputToKeyCode(input);
                modifiedState[keycode] = ACTIVE_KEY;
            }

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
