using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Data
{
    [Serializable]
    public struct ServerParams
    {
        [Serializable]
        public struct RenderParams
        {
            public readonly uint renderWidth;
            public readonly uint renderHeight;
            public readonly bool renderingEnabled;

            public RenderParams(uint renderWidth, uint renderHeight, bool renderingEnabled)
            {
                this.renderWidth = renderWidth;
                this.renderHeight = renderHeight;
                this.renderingEnabled = renderingEnabled;
            }
        }

        private const string serverMutexId = "a";
        private const string clientMutexId = "b";
        private const string returnCodeMemoryId = "0";
        private const string serverInfoMemoryId = "1";
        private const string instructionMemoryId = "2";
        private const string observationMemoryId = "3";
        private const string infoMemoryId = "4";
        private const string rewardMemoryId = "5";
        private const string actionMemoryId = "6";
        private const string terminationMemoryId = "7";

        public readonly bool isRealTime;
        public readonly int frameskip;
        public readonly RenderParams renderParams;
        public readonly string logDirectory;
        public readonly string serverMutexName;
        public readonly string clientMutexName;
        public readonly string returnCodeMemoryName;
        public readonly string serverInfoMemoryName;
        public readonly string instructionMemoryName;
        public readonly string observationMemoryName;
        public readonly string infoMemoryName;
        public readonly string rewardMemoryName;
        public readonly string actionMemoryName;
        public readonly string terminationMemoryName;

        public ServerParams(bool isRealTime, int frameskip, RenderParams renderOptions, string logDirectoryPath, string sharedResourcesPrefix)
        {
            this.isRealTime = isRealTime;
            this.frameskip = frameskip;
            this.renderParams = renderOptions;
            this.logDirectory = logDirectoryPath;
            this.serverMutexName = $"{sharedResourcesPrefix}{serverMutexId}";
            this.clientMutexName = $"{sharedResourcesPrefix}{clientMutexId}";
            this.returnCodeMemoryName = $"{sharedResourcesPrefix}{returnCodeMemoryId}";
            this.serverInfoMemoryName = $"{sharedResourcesPrefix}{serverInfoMemoryId}";
            this.instructionMemoryName = $"{sharedResourcesPrefix}{instructionMemoryId}";
            this.observationMemoryName = $"{sharedResourcesPrefix}{observationMemoryId}";
            this.infoMemoryName = $"{sharedResourcesPrefix}{infoMemoryId}";
            this.rewardMemoryName = $"{sharedResourcesPrefix}{rewardMemoryId}";
            this.actionMemoryName = $"{sharedResourcesPrefix}{actionMemoryId}";
            this.terminationMemoryName = $"{sharedResourcesPrefix}{terminationMemoryId}";
        }
    }
}
