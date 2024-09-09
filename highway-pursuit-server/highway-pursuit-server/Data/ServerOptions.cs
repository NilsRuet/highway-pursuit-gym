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
        // TODO: maybe there is a way to read these from a file or something
        private const string server_mutex_id = "a";
        private const string client_mutex_id = "b";
        private const string server_info_memory_id = "1";
        private const string instruction_memory_id = "2";
        private const string observation_memory_id = "3";
        private const string info_memory_id = "4";
        private const string reward_memory_id = "5";
        private const string action_memory_id = "6";

        public readonly bool isRealTime;
        public readonly string serverMutexName;
        public readonly string clientMutexName;
        public readonly string serverInfoMemoryName;
        public readonly string instructionMemoryName;
        public readonly string observationMemoryName;
        public readonly string infoMemoryName;
        public readonly string rewardMemoryName;
        public readonly string actionMemoryName;

        public ServerOptions(bool isRealTime, string sharedResourcesPrefix)
        {
            this.isRealTime = isRealTime;
            this.serverMutexName = $"{sharedResourcesPrefix}{server_mutex_id}";
            this.clientMutexName = $"{sharedResourcesPrefix}{client_mutex_id}";
            this.serverInfoMemoryName = $"{sharedResourcesPrefix}{server_info_memory_id}";
            this.instructionMemoryName = $"{sharedResourcesPrefix}{instruction_memory_id}";
            this.observationMemoryName = $"{sharedResourcesPrefix}{observation_memory_id}";
            this.infoMemoryName = $"{sharedResourcesPrefix}{info_memory_id}";
            this.rewardMemoryName = $"{sharedResourcesPrefix}{reward_memory_id}";
            this.actionMemoryName = $"{sharedResourcesPrefix}{action_memory_id}";
        }
    }
}
