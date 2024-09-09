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
        public uint obsHeight;
        public uint obsWidth;
        public uint obsChannels;
        public uint actionCount;
        public ServerInfo(uint obsHeight, uint obsWidth, uint obsChannels, uint actionCount)
        {
            this.obsHeight = obsHeight;
            this.obsWidth = obsWidth;
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
        public InstructionCode code;
        public Instruction(InstructionCode code)
        {
            this.code = code;
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
        public float reward;
        public Reward(float reward)
        {
            this.reward = reward;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Termination
    {
        public byte terminated;
        public byte truncated;
        public Termination(byte terminated, byte truncated)
        {
            this.terminated = terminated;
            this.truncated = truncated;
        }
    }
}
