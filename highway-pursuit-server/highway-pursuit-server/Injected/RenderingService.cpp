#include "../pch.h"
#include "RenderingService.hpp"
#include "MemoryAddresses.hpp"

namespace Injected
{
    RenderingService* RenderingService::Instance = nullptr;

    RenderingService::RenderingService(std::shared_ptr<HookManager> hookManager, const ServerParams::RenderParams& renderParams) :
        _hookManager(hookManager),
        _renderParams(renderParams)
    {
        this->FindAddresses();
        this->RegisterHooks();
    }

    // This fetches all adresses from the d3d8 dll, which depend on the windows version
    void RenderingService::FindAddresses()
    {
        // Create a minimalistic invisible window, to get a valid handle
        MinimalWindow mappingWindow;

        // Find d3d8 create
        IDirect3D8_Base::Direct3DCreate8 = reinterpret_cast<Direct3DCreate8_t>(GetProcAddress(_hookManager->GetD3D8(), "Direct3DCreate8"));
        if (IDirect3D8_Base::Direct3DCreate8 == nullptr)
        {
            throw std::runtime_error("Failed to get Direct3DCreate8");
        }

        // Create a D3D8 instance
        D3D8Wrapper d3d8(HighwayPursuitConstants::D3D8_SDK_VERSION);

        // Find d3d8 functions
        uintptr_t* d3d8_vtable = *reinterpret_cast<uintptr_t**>(d3d8.D3D8());
        IDirect3D8_Base::GetAdapterDisplayMode = reinterpret_cast<GetAdapterDisplayMode_t>(d3d8_vtable[MemoryAddresses::GET_ADAPATER_DISPLAY_MODE_OFFSET]);
        IDirect3D8_Base::CreateDevice = reinterpret_cast<CreateDevice_t>(d3d8_vtable[MemoryAddresses::CREATE_DEVICE_OFFSET]);
        IDirect3D8_Base::Release = reinterpret_cast<Direct3DRelease_t>(d3d8_vtable[MemoryAddresses::D3D8_RELEASE_OFFSET]);

        // Device params
        D3DDISPLAYMODE displayMode;
        HandleD3DERR(IDirect3D8_Base::GetAdapterDisplayMode(d3d8.D3D8(), 0, &displayMode));

        D3DPRESENT_PARAMETERS parameters;
        parameters.hDeviceWindow = mappingWindow.Handle();
        parameters.SwapEffect = 1;
        parameters.BackBufferCount = 1;
        parameters.EnableAutoDepthStencil = 1;
        parameters.AutoDepthStencilFormat = D3DFORMAT::D3DFMT_D16;
        parameters.Windowed = 1;
        parameters.BackBufferFormat = displayMode.Format;
        parameters.MultiSampleType = 0;
        parameters.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

        // Create device
        D3D8DeviceWrapper device(d3d8.D3D8(), 0, D3DDEVTYPE_HAL, mappingWindow.Handle(), 0x40, &parameters);

        // Find device functions
        uintptr_t* d3ddevice_vtable = *reinterpret_cast<uintptr_t**>(device.Device());
        IDirect3DDevice8_Base::GetBackBuffer = reinterpret_cast<GetBackBuffer_t>(d3ddevice_vtable[MemoryAddresses::GET_BACK_BUFFER_OFFSET]);
        IDirect3DDevice8_Base::Reset = reinterpret_cast<Reset_t>(d3ddevice_vtable[MemoryAddresses::RESET_DEVICE_OFFSET]);
        IDirect3DDevice8_Base::Present = reinterpret_cast<Present_t>(d3ddevice_vtable[MemoryAddresses::PRESENT_OFFSET]);
        IDirect3DDevice8_Base::Release = reinterpret_cast<DeviceRelease_t>(d3ddevice_vtable[MemoryAddresses::DEVICE_RELEASE_OFFSET]);

        // Call get back buffer to create a surface
        D3D8BackBufferSurfaceWrapper surface(device.Device(), 0, D3DBACKBUFFER_TYPE::MONO);

        // Find surface functions
        uintptr_t* d3dsurface_vtable = *reinterpret_cast<uintptr_t**>(surface.Surface());
        IDirect3DSurface8_Base::GetDesc = reinterpret_cast<GetDesc_t>(d3dsurface_vtable[MemoryAddresses::GET_DESC_OFFSET]);
        IDirect3DSurface8_Base::LockRect = reinterpret_cast<LockRect_t>(d3dsurface_vtable[MemoryAddresses::LOCK_RECT_OFFSET]);
        IDirect3DSurface8_Base::UnlockRect = reinterpret_cast<UnlockRect_t>(d3dsurface_vtable[MemoryAddresses::UNLOCK_RECT_OFFSET]);
        IDirect3DSurface8_Base::Release = reinterpret_cast<SurfaceRelease_t>(d3dsurface_vtable[MemoryAddresses::SURFACE_RELEASE_OFFSET]);
    }

    void RenderingService::RegisterHooks()
    {
        // Setup pointers for static hooks
        RenderingService::Instance = this;

        // D3D8 functions
        _hookManager->RegisterHook(IDirect3D8_Base::CreateDevice, &CreateDevice_StaticHook, &IDirect3D8_Base::CreateDevice);
        _hookManager->RegisterHook(IDirect3DDevice8_Base::Reset, &Reset_StaticHook, &IDirect3DDevice8_Base::Reset);
        _hookManager->RegisterHook(IDirect3DDevice8_Base::Present, &Present_StaticHook, &IDirect3DDevice8_Base::Present);
    }

    void RenderingService::HandleD3DERR(D3DERR errorCode)
    {
        if (errorCode > 0)
        {
            throw D3D8Exception(errorCode);
        }
    }

    BufferFormat RenderingService::GetBufferFormat()
    {
        // Get the back buffer surface
        D3D8BackBufferSurfaceWrapper surface(Device(), 0, D3DBACKBUFFER_TYPE::MONO);
        return GetBufferFormatFromSurface(surface.Surface());
    }

    void RenderingService::SetFullscreenFlag(bool useFullscreen)
    {
        BYTE* fullscreenFlagP = reinterpret_cast<BYTE*>(_hookManager->GetModuleBase() + MemoryAddresses::FULLSCREEN_FLAG_OFFSET);
        *fullscreenFlagP = useFullscreen ? 0x1 : 0x0;
    }

    void RenderingService::ResetZoomLevel()
    {
        float* cameraZoom = reinterpret_cast<float*>(_hookManager->GetModuleBase() + MemoryAddresses::CAMERA_ZOOM_ANIM_OFFSET);
        *cameraZoom = FULL_ZOOM;
    }

    void RenderingService::Screenshot(std::function<void(void*, const BufferFormat&)> pixelDataHandler)
    {
        // Back buffer method
        D3D8BackBufferSurfaceWrapper backbuffer(Device(), 0, D3DBACKBUFFER_TYPE::MONO);
        BufferFormat format = GetBufferFormatFromSurface(backbuffer.Surface());
        RECT renderingRect{ 0,0,static_cast<LONG>(format.width), static_cast<LONG>(format.height) };

        // Lock pixels
        D3D8LockedRectWrapper lockedRect(backbuffer.Surface(), &renderingRect, LOCK_RECT_FLAGS::D3DLOCK_READONLY);
        pixelDataHandler(lockedRect.Rect()->pBits, format);
    }


    BufferFormat RenderingService::GetBufferFormatFromSurface(IDirect3DSurface8* pSurface)
    {
        D3DSURFACE_DESC desc;
        HandleD3DERR(IDirect3DSurface8_Base::GetDesc(pSurface, &desc));
        return BufferFormat(desc);
    }

    IDirect3DDevice8* RenderingService::Device()
    {
        IDirect3DDevice8** ppDevice = reinterpret_cast<IDirect3DDevice8**>(_hookManager->GetModuleBase() + MemoryAddresses::DEVICE_PTR_OFFSET);
        return *ppDevice;
    }

    D3DERR RenderingService::CreateDevice_Hook(IDirect3D8* pD3D8, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface)
    {
        UpdatePresentationParams(pPresentationParameters);
        auto res = IDirect3D8_Base::CreateDevice(pD3D8, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
        return res;
    }

    D3DERR RenderingService::Reset_Hook(IDirect3DDevice8* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
        UpdatePresentationParams(pPresentationParameters);
        return IDirect3DDevice8_Base::Reset(pDevice, pPresentationParameters);
    }

    D3DERR RenderingService::Present_Hook(IDirect3DDevice8* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
    {
        if (_renderParams.renderingEnabled)
        {
            return IDirect3DDevice8_Base::Present(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
        }
        else
        {
            return D3DERR_OK; // Don't do anything
        }
    }

    void RenderingService::UpdatePresentationParams(D3DPRESENT_PARAMETERS* pPresentationParameters) const
    {
        // Make back buffer lockable for pixel capture && update width/height
        pPresentationParameters->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
        pPresentationParameters->BackBufferWidth = this->_renderParams.renderWidth;
        pPresentationParameters->BackBufferHeight = this->_renderParams.renderHeight;
    }


    D3DERR __stdcall RenderingService::CreateDevice_StaticHook(IDirect3D8* pD3D8, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface)
    {
        if (RenderingService::Instance != nullptr)
        {
            return RenderingService::Instance->CreateDevice_Hook(pD3D8, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
        }
        return D3DERR_NOTAVAILABLE;
    }

    D3DERR __stdcall RenderingService::Reset_StaticHook(IDirect3DDevice8* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
        if (RenderingService::Instance != nullptr)
        {
            return RenderingService::Instance->Reset_Hook(pDevice, pPresentationParameters);
        }
        return D3DERR_NOTAVAILABLE;
    }

    D3DERR __stdcall RenderingService::Present_StaticHook(IDirect3DDevice8* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
    {
        if (RenderingService::Instance != nullptr)
        {
            return RenderingService::Instance->Present_Hook(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
        }
        return D3DERR_NOTAVAILABLE;
    }


    // Inner classes for managed resources
    // Window wrapper
    RenderingService::MinimalWindow::MinimalWindow()
    {
        // Define a minimal window class
        const char CLASS_NAME[] = "MinimalWindow";
        HMODULE hInstance = GetModuleHandle(NULL);
        WNDCLASSA wc = { };
        wc.lpfnWndProc = DefWindowProcA;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;

        // Register the window class
        RegisterClassA(&wc);

        // Create a simple window
        HWND hwnd = CreateWindowExA(
            0,
            CLASS_NAME,
            "Minimal window",
            WS_POPUP,
            0, 0, 1, 1, // Small position/size, irrelevant since hidden
            NULL,
            NULL,
            hInstance,
            NULL
        );

        // Check the handle
        if (hwnd == NULL)
        {
            throw std::runtime_error("Failed to create minimal window");
        }

        _handle = hwnd;
    }

    RenderingService::MinimalWindow::~MinimalWindow()
    {
        DestroyWindow(_handle);
    }

    HWND RenderingService::MinimalWindow::Handle() const
    {
        return _handle;
    }

    // D3D8 wrapper
    RenderingService::D3D8Wrapper::D3D8Wrapper(UINT SDK_VERSION)
    {
        // Create d3d8
        _d3d8 = IDirect3D8_Base::Direct3DCreate8(SDK_VERSION);
        if (_d3d8 == nullptr)
        {
            throw std::runtime_error("Failed to create Direct3D");
        }
    }

    RenderingService::D3D8Wrapper::~D3D8Wrapper()
    {
        HandleD3DERR(IDirect3D8_Base::Release(_d3d8));
    }

    IDirect3D8* RenderingService::D3D8Wrapper::D3D8() const
    {
        return _d3d8;
    }

    // Device wrapper
    RenderingService::D3D8DeviceWrapper::D3D8DeviceWrapper(IDirect3D8* d3d8, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
        IDirect3DDevice8* pDevice;
        HandleD3DERR(IDirect3D8_Base::CreateDevice(d3d8, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, &pDevice));
        _pDevice = pDevice;
    }

    RenderingService::D3D8DeviceWrapper::~D3D8DeviceWrapper()
    {
        HandleD3DERR(IDirect3DDevice8_Base::Release(_pDevice));
    }

    IDirect3DSurface8* RenderingService::D3D8DeviceWrapper::Device() const
    {
        return _pDevice;
    }

    // Surface wrapper
    RenderingService::D3D8BackBufferSurfaceWrapper::D3D8BackBufferSurfaceWrapper(IDirect3DDevice8* pDevice, UINT BackBuffer, D3DBACKBUFFER_TYPE Type)
    {
        IDirect3DSurface8* pSurface;
        HandleD3DERR(IDirect3DDevice8_Base::GetBackBuffer(pDevice, BackBuffer, Type, &pSurface));
        _pSurface = pSurface;
    }

    RenderingService::D3D8BackBufferSurfaceWrapper::~D3D8BackBufferSurfaceWrapper()
    {
        HandleD3DERR(IDirect3DSurface8_Base::Release(_pSurface));
    }

    IDirect3DSurface8* RenderingService::D3D8BackBufferSurfaceWrapper::Surface() const
    {
        return _pSurface;
    }

    RenderingService::D3D8LockedRectWrapper::D3D8LockedRectWrapper(IDirect3DSurface8* pSurface, CONST RECT* pRect, DWORD Flags) : _pSurface(pSurface)
    {
        _lockedRect = std::make_shared<D3DLOCKED_RECT>();
        HandleD3DERR(IDirect3DSurface8_Base::LockRect(_pSurface, _lockedRect.get(), pRect, Flags));
    }

    RenderingService::D3D8LockedRectWrapper::~D3D8LockedRectWrapper()
    {
        HandleD3DERR(IDirect3DSurface8_Base::UnlockRect(_pSurface));
    }

    std::shared_ptr<D3DLOCKED_RECT> RenderingService::D3D8LockedRectWrapper::Rect()
    {
        return _lockedRect;
    }

    // Init static members
    RenderingService::GetAdapterDisplayMode_t RenderingService::IDirect3D8_Base::GetAdapterDisplayMode = nullptr;
    RenderingService::CreateDevice_t RenderingService::IDirect3D8_Base::CreateDevice = nullptr;
    RenderingService::Direct3DCreate8_t RenderingService::IDirect3D8_Base::Direct3DCreate8 = nullptr;
    RenderingService::Direct3DRelease_t RenderingService::IDirect3D8_Base::Release = nullptr;

    RenderingService::Reset_t RenderingService::IDirect3DDevice8_Base::Reset = nullptr;
    RenderingService::Present_t RenderingService::IDirect3DDevice8_Base::Present = nullptr;
    RenderingService::GetBackBuffer_t RenderingService::IDirect3DDevice8_Base::GetBackBuffer = nullptr;
    RenderingService::DeviceRelease_t RenderingService::IDirect3DDevice8_Base::Release = nullptr;

    RenderingService::GetDesc_t RenderingService::IDirect3DSurface8_Base::GetDesc = nullptr;
    RenderingService::LockRect_t RenderingService::IDirect3DSurface8_Base::LockRect = nullptr;
    RenderingService::UnlockRect_t RenderingService::IDirect3DSurface8_Base::UnlockRect = nullptr;
    RenderingService::SurfaceRelease_t RenderingService::IDirect3DSurface8_Base::Release = nullptr;
}
