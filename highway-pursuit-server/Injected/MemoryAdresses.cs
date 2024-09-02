using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HighwayPursuitServer.Injected
{
    static class MemoryAdresses
    {
        #region HighwayPursuit.exe
        public const uint UPDATE_OFFSET = 0x29420;
        public const uint GET_LIFE_COUNT_OFFSET = 0x15C70;
        public const uint SET_SCORE_OFFSET = 0x15C00;
        public const uint DEVICE_PTR_OFFSET = 0x960CC;
        #endregion
        #region d3d8.dll
        public const uint GET_DISPLAY_MODE_OFFSET = 0x2A650;
        public const uint CREATE_SURFACE_IMAGE_OFFSET = 0x2B6F0;
        public const uint GET_BACK_BUFFER_OFFSET = 0x2A1F0;
        public const uint COPY_RECTS_OFFSET = 0x2A880;
        public const uint LOCK_RECT_OFFSET = 0x27E50;
        public const uint UNLOCK_RECT_OFFSET = 0x27F30;
        public const uint SURFACE_RELEASE_OFFSET = 0x24140;
        #endregion
    }
}
