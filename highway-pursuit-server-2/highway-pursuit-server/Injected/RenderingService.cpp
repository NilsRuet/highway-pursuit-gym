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
        RegisterHooks();
    }

    void RenderingService::RegisterHooks()
    {
        // Setup pointers for static hooks
        RenderingService::Instance = this;

        // Window Procedure
        LPVOID windowProcPtr = reinterpret_cast<LPVOID>(_hookManager->GetModuleBase() + MemoryAddresses::WINDOW_PROC_OFFSET);
        _hookManager->RegisterHook(windowProcPtr, &WindowProcedure_StaticHook, &WindowProcedure_Base);

        // D3D8 functions
        uintptr_t d3d8 = _hookManager->GetD3D8Base();

        // Create device
        LPVOID createDevicePtr = reinterpret_cast<LPVOID>(d3d8 + MemoryAddresses::CREATE_DEVICE_OFFSET);
        _hookManager->RegisterHook(createDevicePtr, &CreateDevice_StaticHook, &IDirect3D8_Base::CreateDevice);

        // Reset device
        LPVOID resetDevicePtr = reinterpret_cast<LPVOID>(d3d8 + MemoryAddresses::RESET_DEVICE_OFFSET);
        _hookManager->RegisterHook(resetDevicePtr, &Reset_StaticHook, &IDirect3DDevice8_Base::Reset);

        // Present device
        LPVOID presentPtr = reinterpret_cast<LPVOID>(d3d8 + MemoryAddresses::PRESENT_OFFSET);
        _hookManager->RegisterHook(presentPtr, &Present_StaticHook, &IDirect3DDevice8_Base::Present);

        // Device called functions
        IDirect3DDevice8_Base::GetDisplayMode = (GetDisplayMode_t)(d3d8 + MemoryAddresses::RESET_DEVICE_OFFSET);
        IDirect3DDevice8_Base::CreateImageSurface = (CreateImageSurface_t)(d3d8 + MemoryAddresses::CREATE_SURFACE_IMAGE_OFFSET);
        IDirect3DDevice8_Base::GetBackBuffer = (GetBackBuffer_t)(d3d8 + MemoryAddresses::GET_BACK_BUFFER_OFFSET);
        IDirect3DDevice8_Base::CopyRects = (CopyRects_t)(d3d8 + MemoryAddresses::COPY_RECTS_OFFSET);

        // Surface functions
        IDirect3DSurface8_Base::GetDesc = (GetDesc_t)(d3d8 + MemoryAddresses::GET_DESC_OFFSET);
        IDirect3DSurface8_Base::LockRect = (LockRect_t)(d3d8 + MemoryAddresses::LOCK_RECT_OFFSET);
        IDirect3DSurface8_Base::UnlockRect = (UnlockRect_t)(d3d8 + MemoryAddresses::UNLOCK_RECT_OFFSET);
        IDirect3DSurface8_Base::Release = (Release_t)(d3d8 + MemoryAddresses::SURFACE_RELEASE_OFFSET);
    }

    void RenderingService::HandleDRDERR(D3DERR errorCode)
    {
        if (errorCode > 0)
        {
            throw D3D8Exception(errorCode);
        }
    }

    BufferFormat RenderingService::GetBufferFormat()
    {
        BufferFormat res;
        // Get the back buffer surface
        IDirect3DSurface8* pBackBufferSurface = nullptr;
        HandleDRDERR(IDirect3DDevice8_Base::GetBackBuffer(Device(), 0, D3DBACKBUFFER_TYPE::MONO, &pBackBufferSurface));
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
        HandleDRDERR(IDirect3DDevice8_Base::GetBackBuffer(Device(), 0, D3DBACKBUFFER_TYPE::MONO, &pBackBufferSurface));
        BufferFormat format = GetBufferFormatFromSurface(pBackBufferSurface);
        RECT renderingRect = { 0,0,static_cast<LONG>(format.width), static_cast<LONG>(format.height) };

        try
        {
            // Lock pixels
            D3DLOCKED_RECT lockedRect;
            HandleDRDERR(IDirect3DSurface8_Base::LockRect(pBackBufferSurface, &lockedRect, &renderingRect, LOCK_RECT_FLAGS::D3DLOCK_READONLY));
            pixelDataHandler(lockedRect.pBits, format);
        }
        catch(...)
        {
            // Release resources
            HandleDRDERR(IDirect3DSurface8_Base::UnlockRect(pBackBufferSurface));
            HandleDRDERR(IDirect3DSurface8_Base::Release(pBackBufferSurface));
            throw;
        }
        HandleDRDERR(IDirect3DSurface8_Base::UnlockRect(pBackBufferSurface));
        HandleDRDERR(IDirect3DSurface8_Base::Release(pBackBufferSurface));
    }


    BufferFormat RenderingService::GetBufferFormatFromSurface(IDirect3DSurface8* pSurface)
    {
        D3DSURFACE_DESC* pDesc = nullptr;
        HandleDRDERR(IDirect3DSurface8_Base::GetDesc(pSurface, pDesc));
        return BufferFormat(pDesc);
    }

    IDirect3DDevice8* RenderingService::Device()
    {
        IDirect3DDevice8** ppDevice = reinterpret_cast<IDirect3DDevice8**>(_hookManager->GetD3D8Base() + MemoryAddresses::DEVICE_PTR_OFFSET);
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
        return IDirect3D8_Base::CreateDevice(pD3D8, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
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
        const uint32_t D3DPRESENTFLAG_LOCKABLE_BACKBUFFER = 0x00000001;
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
    RenderingService::CreateDevice_t RenderingService::IDirect3D8_Base::CreateDevice = nullptr;
    RenderingService::Reset_t RenderingService::IDirect3DDevice8_Base::Reset = nullptr;
    RenderingService::GetDisplayMode_t RenderingService::IDirect3DDevice8_Base::GetDisplayMode = nullptr;
    RenderingService::Present_t RenderingService::IDirect3DDevice8_Base::Present = nullptr;
    RenderingService::CreateImageSurface_t RenderingService::IDirect3DDevice8_Base::CreateImageSurface = nullptr;
    RenderingService::GetBackBuffer_t RenderingService::IDirect3DDevice8_Base::GetBackBuffer = nullptr;
    RenderingService::CopyRects_t RenderingService::IDirect3DDevice8_Base::CopyRects = nullptr;
    RenderingService::GetDesc_t RenderingService::IDirect3DSurface8_Base::GetDesc = nullptr;
    RenderingService::LockRect_t RenderingService::IDirect3DSurface8_Base::LockRect = nullptr;
    RenderingService::UnlockRect_t RenderingService::IDirect3DSurface8_Base::UnlockRect = nullptr;
    RenderingService::Release_t RenderingService::IDirect3DSurface8_Base::Release = nullptr;
}
