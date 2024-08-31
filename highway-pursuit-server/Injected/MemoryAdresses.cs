using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    static class MemoryAdresses
    {
        #region Highway pursuit
        public const uint UPDATE_OFFSET = 0x29420;
        public const uint GET_LIFE_COUNT_OFFSET = 0x15C70;
        public const uint DEVICE_PTR_OFFSET = 0x960CC;
        #endregion
        #region d3d8.dll
        public const uint CREATE_SURFACE_IMAGE_OFFSET = 0x2B6F0;
        #endregion
    }
}
