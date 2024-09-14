using HighwayPursuitServer.Server;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    class ScoreService
    {
        private readonly IHookManager _hookManager;
        private int _lastScore = 0;
        private int _newScore = 0;

        public ScoreService(IHookManager hookManager)
        {
            this._hookManager = hookManager;
            this.RegisterHooks();
        }

        public int PullReward()
        {
            var scoreDelta = _newScore - _lastScore;
            _lastScore = _newScore;
            return scoreDelta;
        }

        public void ResetScore()
        {
            _lastScore = 0;
            _newScore = 0;
        }

        #region Hooking
        private void RegisterHooks()
        {
            IntPtr setScorePtr = new IntPtr(_hookManager.GetModuleBase().ToInt32() + MemoryAdresses.SET_SCORE_OFFSET);
            SetScore = Marshal.GetDelegateForFunctionPointer<SetScore_delegate>(setScorePtr);
            _hookManager.RegisterHook(setScorePtr, new SetScore_delegate(SetScore_Hook));
        }

        #region hooks
        void SetScore_Hook(int score)
        {
            SetScore(score);
            _newScore = score;
        }
        #endregion

        #region delegates
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void SetScore_delegate(int score);
        #endregion

        #region original function pointers
        static SetScore_delegate SetScore;
        #endregion
        #endregion
    }
}
