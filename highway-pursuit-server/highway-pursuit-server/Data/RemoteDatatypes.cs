using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Data
{
    [StructLayout(LayoutKind.Sequential)]
    public struct ServerInfo
    {
        public uint obsWidth;
        public uint obsHeight;
        public uint obsChannels;
        public uint actionCount;

        public ServerInfo(uint obsWidth, uint obsHeight, uint obsChannels, uint actionCount)
        {
            this.obsWidth = obsWidth;
            this.obsHeight = obsHeight;
            this.obsChannels = obsChannels;
            this.actionCount = actionCount;
        }
    }

    public enum InstructionCode : uint
    {
        RESET = 1,
        STEP = 2,
        CLOSE = 3,
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Instruction
    {
        public InstructionCode instruction;

        public Instruction(InstructionCode instruction)
        {
            this.instruction = instruction;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Info
    {
        public float tps;
        public float memory;

        public Info(float tps, float memory)
        {
            this.tps = tps;
            this.memory = memory;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Reward
    {
        public uint reward;
        public Reward(uint reward)
        {
            this.reward = reward;
        }
    }
}
