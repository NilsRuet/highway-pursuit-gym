using HighwayPursuitServer.Server;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    class ResetService
    {
        private readonly IHookManager _hookManager;
        public ResetService(IHookManager hookManager)
        {
            this._hookManager = hookManager;
        }

        public void Reset()
        {

        }
    }
}
