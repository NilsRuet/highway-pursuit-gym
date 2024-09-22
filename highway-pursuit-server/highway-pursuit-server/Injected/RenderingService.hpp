#pragma once
#include "../Data/ServerTypes.hpp"
#include "../Data/D3D8.hpp"
#include "../HookManager.hpp"

namespace Injected
{
    using namespace Data;

	class RenderingService
	{
    public:
        // Static instance ptr
        static RenderingService* Instance;
        static constexpr float FULL_ZOOM = 10.0f;

        // Constructor
        RenderingService(std::shared_ptr<HookManager> hookManager, const ServerParams::RenderParams& renderParams);

        // Methods
        BufferFormat GetBufferFormat();
        void SetFullscreenFlag(bool useFullscreen);
        void ResetZoomLevel();
        void Screenshot(std::function<void(void*, const BufferFormat&)> pixelDataHandler);


    private:
        // Members
        std::shared_ptr<HookManager> _hookManager;
        Data::ServerParams::RenderParams _renderParams;

        // private methods
        void RegisterHooks();
        void HandleDRDERR(D3DERR errorCode);
        BufferFormat GetBufferFormatFromSurface(IDirect3DSurface8* pSurface);
        IDirect3DDevice8* Device();

        // Hooks
        void WindowProcedure_Hook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        D3DERR CreateDevice_Hook(IDirect3D8* pD3D8, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface);
        D3DERR Reset_Hook(IDirect3DDevice8* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
        D3DERR Present_Hook(IDirect3DDevice8* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
        void UpdatePresentationParams(D3DPRESENT_PARAMETERS* pPresentationParameters) const;

        // Function signatures
        typedef void (__stdcall* WindowProcedure_t)(HWND, UINT, WPARAM, LPARAM);
        typedef D3DERR(__stdcall* CreateDevice_t)(IDirect3D8*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice8**);
        // Device
        typedef D3DERR(__stdcall* Reset_t)(IDirect3DDevice8*, D3DPRESENT_PARAMETERS*);
        typedef D3DERR(__stdcall* GetDisplayMode_t)(IDirect3DDevice8*, D3DDISPLAYMODE*);
        typedef D3DERR(__stdcall* Present_t)(IDirect3DDevice8*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
        typedef D3DERR(__stdcall* CreateImageSurface_t)(IDirect3DDevice8*, UINT, UINT, D3DFORMAT, IDirect3DSurface8**);
        typedef D3DERR(__stdcall* GetBackBuffer_t)(IDirect3DDevice8*, UINT, D3DBACKBUFFER_TYPE, IDirect3DSurface8**);
        typedef D3DERR(__stdcall* CopyRects_t)(IDirect3DDevice8*, IDirect3DSurface8*, CONST RECT*, UINT, IDirect3DSurface8*, CONST POINT*);

        // Surface methods
        typedef D3DERR(__stdcall* GetDesc_t)(IDirect3DSurface8*, D3DSURFACE_DESC*);
        typedef D3DERR(__stdcall* LockRect_t)(IDirect3DSurface8*, D3DLOCKED_RECT*, CONST RECT*, DWORD);
        typedef D3DERR(__stdcall* UnlockRect_t)(IDirect3DSurface8*);
        typedef D3DERR(__stdcall* Release_t)(IDirect3DSurface8*);

        // Static hooks (entry points)
        static void __stdcall WindowProcedure_StaticHook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static D3DERR __stdcall CreateDevice_StaticHook(IDirect3D8* pD3D8, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface);
        static D3DERR __stdcall Reset_StaticHook(IDirect3DDevice8* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
        static D3DERR __stdcall Present_StaticHook(IDirect3DDevice8* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
	
        // Original functions
        static WindowProcedure_t WindowProcedure_Base;

        class IDirect3D8_Base
        {
        public:
            static CreateDevice_t CreateDevice;
        };
        class IDirect3DDevice8_Base
        {
        public:
            static Reset_t Reset;
            static GetDisplayMode_t GetDisplayMode;
            static Present_t Present;
            static CreateImageSurface_t CreateImageSurface;
            static GetBackBuffer_t GetBackBuffer;
            static CopyRects_t CopyRects;
        };

        class IDirect3DSurface8_Base
        {
        public:
            static GetDesc_t GetDesc;
            static LockRect_t LockRect;
            static UnlockRect_t UnlockRect;
            static Release_t Release;
        };
    };
}

