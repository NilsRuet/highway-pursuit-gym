using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Data
{
    public enum Input : uint
    {
        Accelerate,
        Brake,
        SteerL,
        SteerR,
        Fire,
        Oil,
        Smoke,
        Missiles
    }

    static class InputUtils
    {
        public static Input IndexToInput(int index)
        {
            switch (index)
            {
                case 0:
                    return Input.Accelerate;
                case 1:
                    return Input.Brake;
                case 2:
                    return Input.SteerL;
                case 3:
                    return Input.SteerR;
                case 4:
                    return Input.Fire;
                case 5:
                    return Input.Oil;
                case 6:
                    return Input.Smoke;
                case 7:
                    return Input.Missiles;
                default: // TODO this should be an exception
                    return Input.Accelerate;
            }
        }
    }
}
