using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Data
{
    [Serializable]
    public struct ServerOptions
    {
        public readonly bool isRealTime;
        public readonly string serverMutexName;
        public readonly string clientMutexName;
        public readonly string serverInfoMemoryName;
        public readonly string instructionMemoryName;
        public readonly string observationMemoryName;
        public readonly string infoMemoryName;
        public readonly string rewardMemoryName;

        public ServerOptions(bool isRealTime, string serverMutexName, string clientMutexName, string serverInfoMemoryName, string instructionMemoryName, string observationMemoryName, string infoMemoryName, string rewardMemoryName)
        {
            this.isRealTime = isRealTime;
            this.serverMutexName = serverMutexName;
            this.clientMutexName = clientMutexName;
            this.serverInfoMemoryName = serverInfoMemoryName;
            this.instructionMemoryName = instructionMemoryName;
            this.observationMemoryName = observationMemoryName;
            this.infoMemoryName = infoMemoryName;
            this.rewardMemoryName = rewardMemoryName;
        }
    }
}
