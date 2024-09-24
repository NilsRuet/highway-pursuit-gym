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
    }

    // This HAS to be called from the main thread i.e a hook
    void RenderingService::InitFromMainThread(IDirect3D8* d3d8)
    {
        HPLogger::LogDebug("Find addr, d3d8 = "+HPLogger::ToHex((uintptr_t) d3d8));
        // Create a device to retrieve IDirect3DDevice8's vtable
        uintptr_t* d3d8_vtable = *reinterpret_cast<uintptr_t**>(d3d8);
        HPLogger::LogDebug("find vtable device at "+HPLogger::ToHex((uintptr_t)d3d8_vtable));
        IDirect3D8_Base::GetAdapterDisplayMode = reinterpret_cast<GetAdapterDisplayMode_t>(d3d8_vtable[MemoryAddresses::GET_ADAPATER_DISPLAY_MODE_OFFSET]);
        IDirect3D8_Base::CreateDevice = reinterpret_cast<CreateDevice_t>(d3d8_vtable[MemoryAddresses::CREATE_DEVICE_OFFSET]);
        HPLogger::LogDebug("create device at " + HPLogger::ToHex((uintptr_t)IDirect3D8_Base::CreateDevice));
        HPLogger::LogDebug("adapter display mode at " + HPLogger::ToHex((uintptr_t)IDirect3D8_Base::GetAdapterDisplayMode));

        HWND mainWindow = reinterpret_cast<HWND>(_hookManager->GetModuleBase() + MemoryAddresses::MAIN_WINDOW_HANDLE);

        D3DDISPLAYMODE displayMode;
        HandleD3DERR_GameThread(IDirect3D8_Base::GetAdapterDisplayMode(d3d8, 0, &displayMode));
        HPLogger::LogDebug("Display format " + HPLogger::ToHex((uintptr_t)displayMode.Format));

        D3DPRESENT_PARAMETERS parameters;
        parameters.hDeviceWindow = mainWindow;
        parameters.SwapEffect = 1;
        parameters.BackBufferCount = 1;
        parameters.EnableAutoDepthStencil = 1;
        parameters.AutoDepthStencilFormat = D3DFORMAT::D3DFMT_D16;
        parameters.Windowed = 1;
        parameters.BackBufferFormat = displayMode.Format;
        parameters.MultiSampleType = 0;
        parameters.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

        // Try create device
        HPLogger::LogDebug("Create device");
        IDirect3DDevice8* device;
        HandleD3DERR_GameThread(IDirect3D8_Base::CreateDevice(d3d8, 0, D3DDEVTYPE_HAL, mainWindow, 0x40, &parameters, &device));

        HPLogger::LogDebug("Device functions");
        // Device functions
        uintptr_t* d3ddevice_vtable = *reinterpret_cast<uintptr_t**>(device);
        IDirect3DDevice8_Base::GetBackBuffer = reinterpret_cast<GetBackBuffer_t>(d3ddevice_vtable[MemoryAddresses::GET_BACK_BUFFER_OFFSET]);
        IDirect3DDevice8_Base::Reset = reinterpret_cast<Reset_t>(d3ddevice_vtable[MemoryAddresses::RESET_DEVICE_OFFSET]);
        IDirect3DDevice8_Base::Present = reinterpret_cast<Present_t>(d3ddevice_vtable[MemoryAddresses::PRESENT_OFFSET]);
        IDirect3DDevice8_Base::Release = reinterpret_cast<DeviceRelease_t>(d3ddevice_vtable[MemoryAddresses::DEVICE_RELEASE_OFFSET]);
        // Release

        // Call get back buffer to create a surface and access IDirect3DSurface8's vtable
        // Try create surface
        IDirect3DSurface8* surface;
        HandleD3DERR_GameThread(IDirect3DDevice8_Base::GetBackBuffer(device, 0, D3DBACKBUFFER_TYPE::MONO, &surface));

        HPLogger::LogDebug("Surface functions");
        // Surface function
        uintptr_t* d3dsurface_vtable = *reinterpret_cast<uintptr_t**>(surface);
        IDirect3DSurface8_Base::GetDesc = reinterpret_cast<GetDesc_t>(d3dsurface_vtable[MemoryAddresses::GET_DESC_OFFSET]);
        IDirect3DSurface8_Base::LockRect = reinterpret_cast<LockRect_t>(d3dsurface_vtable[MemoryAddresses::LOCK_RECT_OFFSET]);
        IDirect3DSurface8_Base::UnlockRect = reinterpret_cast<UnlockRect_t>(d3dsurface_vtable[MemoryAddresses::UNLOCK_RECT_OFFSET]);
        IDirect3DSurface8_Base::Release = reinterpret_cast<SurfaceRelease_t>(d3dsurface_vtable[MemoryAddresses::SURFACE_RELEASE_OFFSET]);

        // Release surface
        D3DERR releaseSurfaceErr = IDirect3DSurface8_Base::Release(surface);
        // Release device
        D3DERR releaseDeviceErr = IDirect3DDevice8_Base::Release(device);
        HandleD3DERR_GameThread(releaseSurfaceErr);
        HandleD3DERR_GameThread(releaseDeviceErr);
    }

    void RenderingService::Enable()
    {
        this->RegisterHooks();
    }

    void RenderingService::RegisterHooks()
    {
        // Setup pointers for static hooks
        RenderingService::Instance = this;

        // Window Procedure
        LPVOID windowProcPtr = reinterpret_cast<LPVOID>(_hookManager->GetModuleBase() + MemoryAddresses::WINDOW_PROC_OFFSET);
        _hookManager->RegisterHook(windowProcPtr, &WindowProcedure_StaticHook, &WindowProcedure_Base);
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

    void RenderingService::HandleD3DERR_GameThread(D3DERR errorCode)
    {
        if (errorCode > 0)
        {
            HPLogger::LogError(D3D8Exception::FormatErrorMessage(errorCode));
            throw D3D8Exception(errorCode);
        }
    }

    BufferFormat RenderingService::GetBufferFormat()
    {
        BufferFormat res;
        // Get the back buffer surface
        IDirect3DSurface8* pBackBufferSurface = nullptr;
        HandleD3DERR(IDirect3DDevice8_Base::GetBackBuffer(Device(), 0, D3DBACKBUFFER_TYPE::MONO, &pBackBufferSurface));
        try
        {
            // Get the buffer format from the surface
            res = GetBufferFormatFromSurface(pBackBufferSurface);
        }
        catch (...)
        {
            if (pBackBufferSurface)
            {
                IDirect3DSurface8_Base::Release(pBackBufferSurface);
            }
            throw;
        }
        if (pBackBufferSurface)
        {
            IDirect3DSurface8_Base::Release(pBackBufferSurface);
        }
        return res;
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
        IDirect3DSurface8* pBackBufferSurface;
        HandleD3DERR(IDirect3DDevice8_Base::GetBackBuffer(Device(), 0, D3DBACKBUFFER_TYPE::MONO, &pBackBufferSurface));
        BufferFormat format = GetBufferFormatFromSurface(pBackBufferSurface);
        RECT renderingRect = { 0,0,static_cast<LONG>(format.width), static_cast<LONG>(format.height) };

        try
        {
            // Lock pixels
            D3DLOCKED_RECT lockedRect;
            HandleD3DERR(IDirect3DSurface8_Base::LockRect(pBackBufferSurface, &lockedRect, &renderingRect, LOCK_RECT_FLAGS::D3DLOCK_READONLY));
            pixelDataHandler(lockedRect.pBits, format);
        }
        catch(...)
        {
            // Release resources
            HandleD3DERR(IDirect3DSurface8_Base::UnlockRect(pBackBufferSurface));
            HandleD3DERR(IDirect3DSurface8_Base::Release(pBackBufferSurface));
            throw;
        }
        HandleD3DERR(IDirect3DSurface8_Base::UnlockRect(pBackBufferSurface));
        HandleD3DERR(IDirect3DSurface8_Base::Release(pBackBufferSurface));
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

    void RenderingService::WindowProcedure_Hook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        const WPARAM focused = 1;
        // Disable the "lose focus" callback to disable pausing
        if (uMsg == WM_ACTIVATEAPP || uMsg == WM_ACTIVATE)
        {
            WindowProcedure_Base(hwnd, uMsg, focused, lParam);
        }
        else
        {
            WindowProcedure_Base(hwnd, uMsg, wParam, lParam);
        }
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

    // Hook entry points
    void __stdcall RenderingService::WindowProcedure_StaticHook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (RenderingService::Instance != nullptr)
        {
            RenderingService::Instance->WindowProcedure_Hook(hwnd, uMsg, wParam, lParam);
        }
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

    // Init base function pointers for winproc, d3d8, device, surface
    RenderingService::WindowProcedure_t RenderingService::WindowProcedure_Base = nullptr;

    RenderingService::GetAdapterDisplayMode_t RenderingService::IDirect3D8_Base::GetAdapterDisplayMode = nullptr;
    RenderingService::CreateDevice_t RenderingService::IDirect3D8_Base::CreateDevice = nullptr;

    RenderingService::Reset_t RenderingService::IDirect3DDevice8_Base::Reset = nullptr;
    RenderingService::Present_t RenderingService::IDirect3DDevice8_Base::Present = nullptr;
    RenderingService::GetBackBuffer_t RenderingService::IDirect3DDevice8_Base::GetBackBuffer = nullptr;
    RenderingService::DeviceRelease_t RenderingService::IDirect3DDevice8_Base::Release = nullptr;

    RenderingService::GetDesc_t RenderingService::IDirect3DSurface8_Base::GetDesc = nullptr;
    RenderingService::LockRect_t RenderingService::IDirect3DSurface8_Base::LockRect = nullptr;
    RenderingService::UnlockRect_t RenderingService::IDirect3DSurface8_Base::UnlockRect = nullptr;
    RenderingService::SurfaceRelease_t RenderingService::IDirect3DSurface8_Base::Release = nullptr;
}
