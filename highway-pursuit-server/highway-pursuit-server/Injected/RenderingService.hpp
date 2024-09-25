#pragma once
#include "../Data/ServerTypes.hpp"
#include "../Data/D3D8.hpp"
#include "../HookManager.hpp"

namespace Injected
{
    using namespace Data;

	class RenderingService {
    public:
        void FindAddresses();

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
        static void HandleD3DERR(D3DERR errorCode);
        // Members
        std::shared_ptr<HookManager> _hookManager;
        Data::ServerParams::RenderParams _renderParams;

        // Private methods
        void RegisterHooks();
        BufferFormat GetBufferFormatFromSurface(IDirect3DSurface8* pSurface);
        IDirect3DDevice8* Device();

        // Hooks
        D3DERR CreateDevice_Hook(IDirect3D8* pD3D8, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface);
        D3DERR Reset_Hook(IDirect3DDevice8* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
        D3DERR Present_Hook(IDirect3DDevice8* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
        void UpdatePresentationParams(D3DPRESENT_PARAMETERS* pPresentationParameters) const;

        // Function signatures
        typedef IDirect3D8* (WINAPI* Direct3DCreate8_t)(UINT);
        typedef D3DERR(__stdcall* Direct3DRelease_t)(IDirect3D8*);
        // D3D8
        typedef D3DERR(__stdcall* CreateDevice_t)(IDirect3D8*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice8**);
        typedef D3DERR(__stdcall* GetAdapterDisplayMode_t)(IDirect3D8*, UINT, D3DDISPLAYMODE*);
        // Device
        typedef D3DERR(__stdcall* Reset_t)(IDirect3DDevice8*, D3DPRESENT_PARAMETERS*);
        typedef D3DERR(__stdcall* Present_t)(IDirect3DDevice8*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
        typedef D3DERR(__stdcall* GetBackBuffer_t)(IDirect3DDevice8*, UINT, D3DBACKBUFFER_TYPE, IDirect3DSurface8**);
        typedef D3DERR(__stdcall* DeviceRelease_t)(IDirect3DDevice8*);

        // Surface methods
        typedef D3DERR(__stdcall* GetDesc_t)(IDirect3DSurface8*, D3DSURFACE_DESC*);
        typedef D3DERR(__stdcall* LockRect_t)(IDirect3DSurface8*, D3DLOCKED_RECT*, CONST RECT*, DWORD);
        typedef D3DERR(__stdcall* UnlockRect_t)(IDirect3DSurface8*);
        typedef D3DERR(__stdcall* SurfaceRelease_t)(IDirect3DSurface8*);

        // Static hooks (entry points)
        static D3DERR __stdcall CreateDevice_StaticHook(IDirect3D8* pD3D8, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface);
        static D3DERR __stdcall Reset_StaticHook(IDirect3DDevice8* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
        static D3DERR __stdcall Present_StaticHook(IDirect3DDevice8* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
	
        // Original functions
        class IDirect3D8_Base
        {
        public:
            static Direct3DCreate8_t Direct3DCreate8;
            static Direct3DRelease_t Release;
            static CreateDevice_t CreateDevice;
            static GetAdapterDisplayMode_t GetAdapterDisplayMode;
        };
        class IDirect3DDevice8_Base
        {
        public:
            static Reset_t Reset;
            static Present_t Present;
            static GetBackBuffer_t GetBackBuffer;
            static DeviceRelease_t Release;
        };

        class IDirect3DSurface8_Base
        {
        public:
            static GetDesc_t GetDesc;
            static LockRect_t LockRect;
            static UnlockRect_t UnlockRect;
            static SurfaceRelease_t Release;
        };

        // Managed wrapper for some low-level classes
        class MinimalWindow
        {
            HWND _handle;
        public:
            MinimalWindow();
            ~MinimalWindow();
            HWND Handle() const;
        };

        class D3D8Wrapper
        {
            IDirect3D8* _d3d8;
        public:
            D3D8Wrapper(UINT SDK_VERSION);
            ~D3D8Wrapper();
            IDirect3D8* D3D8() const;
        };

        class D3D8DeviceWrapper
        {
            IDirect3DDevice8* _pDevice;
        public:
            D3D8DeviceWrapper(IDirect3D8* d3d8, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters);
            ~D3D8DeviceWrapper();
            IDirect3DSurface8* Device() const;
        };

        class D3D8SurfaceWrapper
        {
            IDirect3DSurface8* _pSurface;
        public:
            D3D8SurfaceWrapper(IDirect3DSurface8* pSurface);
            ~D3D8SurfaceWrapper();
            IDirect3DSurface8* Surface() const;
        };
    };
}

