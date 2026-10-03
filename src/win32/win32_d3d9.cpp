#include "win32_d3d9.h"
#include "../runtime/guest_memory.h"
#include "../renderer/renderer.h"
#include "../platform/platform.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <d3d9.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#include <iostream>
#include <dlfcn.h>
#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" {
extern uint32_t g_eax;
extern uint32_t g_ecx;
extern uint32_t g_edx;
extern uint32_t g_ebx;
extern uint32_t g_esp;
extern uint32_t g_ebp;
extern uint32_t g_esi;
extern uint32_t g_edi;
extern ptrdiff_t g_mem_base;
}

#define ADDR(va) ((uintptr_t)(va) + g_mem_base)
#define MEM32(va) (*reinterpret_cast<uint32_t*>(ADDR(va)))

namespace pcrecomp::win32 {

static void* s_dxvk_handle = nullptr;
typedef IDirect3D9* (*Direct3DCreate9_fn)(UINT SDKVersion);
static Direct3DCreate9_fn s_pfnDirect3DCreate9 = nullptr;

static IDirect3D9* s_host_d3d9 = nullptr;
static IDirect3DDevice9* s_host_d3d9_device = nullptr;

bool init_d3d9_subsystem() {
    // 1. Configure DXVK window system integration (SDL3 driver)
    setenv("DXVK_WSI_DRIVER", "SDL3", 1);

    // 2. Locate and load libdxvk_d3d9.so
    const char* library_paths[] = {
        "thirdparty/dxvk-native/lib/libdxvk_d3d9.so",
        "../thirdparty/dxvk-native/lib/libdxvk_d3d9.so",
        "../../thirdparty/dxvk-native/lib/libdxvk_d3d9.so",
        "libdxvk_d3d9.so",
        nullptr
    };

    for (int i = 0; library_paths[i]; ++i) {
        s_dxvk_handle = dlopen(library_paths[i], RTLD_NOW | RTLD_LOCAL);
        if (s_dxvk_handle) {
            std::cout << "[DXVK] Loaded Direct3D 9 runtime from: " << library_paths[i] << "\n";
            break;
        }
    }

    if (!s_dxvk_handle) {
        std::cerr << "[DXVK] Note: libdxvk_d3d9.so not loaded via dlopen (" << dlerror() << "). Will use static fallback.\n";
    } else {
        s_pfnDirect3DCreate9 = reinterpret_cast<Direct3DCreate9_fn>(dlsym(s_dxvk_handle, "Direct3DCreate9"));
    }

    // 3. Initialize guest VTables and memory structures in guest memory
    if (g_mem_base) {
        // Populate IDirect3D9 guest VTable (17 methods, indices 0..16)
        for (uint32_t i = 0; i < 17; ++i) {
            MEM32(GUEST_D3D9_VTABLE_ADDR + i * 4) = GUEST_D3D9_METHOD_BASE + i * 0x10;
        }

        // Populate IDirect3DDevice9 guest VTable (119 methods, indices 0..118)
        for (uint32_t i = 0; i < 119; ++i) {
            MEM32(GUEST_D3D9DEVICE_VTABLE_ADDR + i * 4) = GUEST_D3D9DEVICE_METHOD_BASE + i * 0x10;
        }

        // Setup 32-bit instance pointers
        MEM32(GUEST_D3D9_INSTANCE_ADDR) = GUEST_D3D9_VTABLE_ADDR;
        MEM32(GUEST_D3D9DEVICE_INSTANCE_ADDR) = GUEST_D3D9DEVICE_VTABLE_ADDR;

        // Initialize RenderWare D3D9 device pointer global
        MEM32(0x00C97C20) = GUEST_D3D9_INSTANCE_ADDR;
    }

    std::cout << "[DXVK] Direct3D 9 guest-to-host COM thunking layer initialized.\n";
    return true;
}

void shutdown_d3d9_subsystem() {
    if (s_host_d3d9_device) {
        s_host_d3d9_device->Release();
        s_host_d3d9_device = nullptr;
    }
    if (s_host_d3d9) {
        s_host_d3d9->Release();
        s_host_d3d9 = nullptr;
    }
    if (s_dxvk_handle) {
        dlclose(s_dxvk_handle);
        s_dxvk_handle = nullptr;
    }
}

uint32_t shim_Direct3DCreate9(uint32_t sdk_version) {
    std::cout << "[DXVK] Direct3DCreate9 called (SDK Version: " << sdk_version << ")\n";

    if (!s_host_d3d9 && s_pfnDirect3DCreate9) {
        s_host_d3d9 = s_pfnDirect3DCreate9(sdk_version);
        if (s_host_d3d9) {
            UINT count = s_host_d3d9->GetAdapterCount();
            std::cout << "[DXVK] IDirect3D9 context created successfully. Active adapters: " << count << "\n";
        } else {
            std::cerr << "[DXVK] Direct3DCreate9 returned NULL.\n";
        }
    }

    // Write guest sentinel into RenderWare global 0x00C97C20
    if (g_mem_base) {
        MEM32(0x00C97C20) = GUEST_D3D9_INSTANCE_ADDR;
    }

    return GUEST_D3D9_INSTANCE_ADDR;
}

// ----------------------------------------------------------------------------
// IDirect3D9 Method Thunks
// ----------------------------------------------------------------------------

void d3d9_thunk_dispatch(uint32_t method_idx) {
    // Standard COM method stdcall calling convention:
    // esp+4 is 'this' pointer (guest instance)
    switch (method_idx) {
        case 0: { // QueryInterface (this, riid, ppvObj) -> purge 12
            uint32_t ppv = MEM32(g_esp + 12);
            if (ppv && g_mem_base) {
                MEM32(ppv) = GUEST_D3D9_INSTANCE_ADDR;
            }
            g_eax = 0; // S_OK
            g_esp += 4 + 12;
            break;
        }
        case 1: { // AddRef (this) -> purge 4
            g_eax = s_host_d3d9 ? s_host_d3d9->AddRef() : 1;
            g_esp += 4 + 4;
            break;
        }
        case 2: { // Release (this) -> purge 4
            g_eax = s_host_d3d9 ? s_host_d3d9->Release() : 0;
            g_esp += 4 + 4;
            break;
        }
        case 4: { // GetAdapterCount (this) -> purge 4
            g_eax = s_host_d3d9 ? s_host_d3d9->GetAdapterCount() : 1;
            g_esp += 4 + 4;
            break;
        }
        case 5: { // GetAdapterIdentifier (this, Adapter, Flags, pIdentifier) -> purge 16
            uint32_t adapter = MEM32(g_esp + 8);
            uint32_t flags = MEM32(g_esp + 12);
            uint32_t pId = MEM32(g_esp + 16);
            if (s_host_d3d9 && pId && g_mem_base) {
                D3DADAPTER_IDENTIFIER9 id{};
                HRESULT hr = s_host_d3d9->GetAdapterIdentifier(adapter, flags, &id);
                std::memcpy(reinterpret_cast<void*>(ADDR(pId)), &id, sizeof(D3DADAPTER_IDENTIFIER9));
                g_eax = static_cast<uint32_t>(hr);
            } else {
                g_eax = 0; // D3D_OK
            }
            g_esp += 4 + 16;
            break;
        }
        case 6: { // GetAdapterModeCount (this, Adapter, Format) -> purge 12
            uint32_t adapter = MEM32(g_esp + 8);
            uint32_t fmt = MEM32(g_esp + 12);
            g_eax = s_host_d3d9 ? s_host_d3d9->GetAdapterModeCount(adapter, static_cast<D3DFORMAT>(fmt)) : 1;
            g_esp += 4 + 12;
            break;
        }
        case 7: { // EnumAdapterModes (this, Adapter, Format, Mode, pMode) -> purge 20
            uint32_t adapter = MEM32(g_esp + 8);
            uint32_t fmt = MEM32(g_esp + 12);
            uint32_t mode_idx = MEM32(g_esp + 16);
            uint32_t pMode = MEM32(g_esp + 20);
            if (s_host_d3d9 && pMode && g_mem_base) {
                D3DDISPLAYMODE mode{};
                HRESULT hr = s_host_d3d9->EnumAdapterModes(adapter, static_cast<D3DFORMAT>(fmt), mode_idx, &mode);
                std::memcpy(reinterpret_cast<void*>(ADDR(pMode)), &mode, sizeof(D3DDISPLAYMODE));
                g_eax = static_cast<uint32_t>(hr);
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 20;
            break;
        }
        case 8: { // GetAdapterDisplayMode (this, Adapter, pMode) -> purge 12
            uint32_t adapter = MEM32(g_esp + 8);
            uint32_t pMode = MEM32(g_esp + 12);
            if (s_host_d3d9 && pMode && g_mem_base) {
                D3DDISPLAYMODE mode{};
                HRESULT hr = s_host_d3d9->GetAdapterDisplayMode(adapter, &mode);
                std::memcpy(reinterpret_cast<void*>(ADDR(pMode)), &mode, sizeof(D3DDISPLAYMODE));
                g_eax = static_cast<uint32_t>(hr);
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 12;
            break;
        }
        case 9: { // CheckDeviceType (this, Adapter, DevType, DispFmt, BBFmt, Windowed) -> purge 24
            g_eax = 0; // D3D_OK
            g_esp += 4 + 24;
            break;
        }
        case 10: { // CheckDeviceFormat (this, Adapter, DevType, AdapterFmt, Usage, RType, CheckFmt) -> purge 28
            g_eax = 0; // D3D_OK
            g_esp += 4 + 28;
            break;
        }
        case 11: { // CheckDeviceMultiSampleType -> purge 28
            g_eax = 0; // D3D_OK
            g_esp += 4 + 28;
            break;
        }
        case 12: { // CheckDepthStencilMatch -> purge 24
            g_eax = 0; // D3D_OK
            g_esp += 4 + 24;
            break;
        }
        case 13: { // CheckDeviceFormatConversion -> purge 20
            g_eax = 0; // D3D_OK
            g_esp += 4 + 20;
            break;
        }
        case 14: { // GetDeviceCaps (this, Adapter, DevType, pCaps) -> purge 16
            uint32_t adapter = MEM32(g_esp + 8);
            uint32_t devType = MEM32(g_esp + 12);
            uint32_t pCaps = MEM32(g_esp + 16);
            if (s_host_d3d9 && pCaps && g_mem_base) {
                D3DCAPS9 caps{};
                HRESULT hr = s_host_d3d9->GetDeviceCaps(adapter, static_cast<D3DDEVTYPE>(devType), &caps);
                std::memcpy(reinterpret_cast<void*>(ADDR(pCaps)), &caps, sizeof(D3DCAPS9));
                g_eax = static_cast<uint32_t>(hr);
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 16;
            break;
        }
        case 15: { // GetAdapterMonitor (this, Adapter) -> purge 8
            g_eax = 0;
            g_esp += 4 + 8;
            break;
        }
        case 16: { // CreateDevice (this, Adapter, DevType, hFocusWindow, BehaviorFlags, pPP, ppDevice) -> purge 28
            uint32_t adapter = MEM32(g_esp + 8);
            uint32_t devType = MEM32(g_esp + 12);
            uint32_t behavior = MEM32(g_esp + 20);
            uint32_t pPP = MEM32(g_esp + 24);
            uint32_t ppDev = MEM32(g_esp + 28);

            void* native_win = platform::Platform::instance().native_window_handle();
            std::cout << "[DXVK] IDirect3D9::CreateDevice called. Native Window: " << native_win << "\n";

            if (s_host_d3d9 && pPP && g_mem_base) {
                D3DPRESENT_PARAMETERS* guest_pp = reinterpret_cast<D3DPRESENT_PARAMETERS*>(ADDR(pPP));
                D3DPRESENT_PARAMETERS host_pp = *guest_pp;
                host_pp.hDeviceWindow = reinterpret_cast<HWND>(native_win);

                HRESULT hr = s_host_d3d9->CreateDevice(
                    adapter,
                    static_cast<D3DDEVTYPE>(devType),
                    reinterpret_cast<HWND>(native_win),
                    behavior,
                    &host_pp,
                    &s_host_d3d9_device
                );

                std::cout << "[DXVK] Host IDirect3DDevice9 creation result: hr=0x" << std::hex << hr << std::dec << "\n";
                if (SUCCEEDED(hr) && s_host_d3d9_device) {
                    *guest_pp = host_pp;
                }
            }

            if (ppDev && g_mem_base) {
                MEM32(ppDev) = GUEST_D3D9DEVICE_INSTANCE_ADDR;
            }
            MEM32(0x00C97C20) = GUEST_D3D9DEVICE_INSTANCE_ADDR;

            g_eax = 0; // D3D_OK
            g_esp += 4 + 28;
            break;
        }
        default: {
            std::cerr << "[DXVK] Unhandled IDirect3D9 method index " << method_idx << "\n";
            g_eax = 0;
            g_esp += 4;
            break;
        }
    }
}

// ----------------------------------------------------------------------------
// IDirect3DDevice9 Method Thunks
// ----------------------------------------------------------------------------

void d3d9_device_thunk_dispatch(uint32_t method_idx) {
    switch (method_idx) {
        case 0: { // QueryInterface -> purge 12
            uint32_t ppv = MEM32(g_esp + 12);
            if (ppv && g_mem_base) MEM32(ppv) = GUEST_D3D9DEVICE_INSTANCE_ADDR;
            g_eax = 0;
            g_esp += 4 + 12;
            break;
        }
        case 1: { // AddRef -> purge 4
            g_eax = s_host_d3d9_device ? s_host_d3d9_device->AddRef() : 1;
            g_esp += 4 + 4;
            break;
        }
        case 2: { // Release -> purge 4
            g_eax = s_host_d3d9_device ? s_host_d3d9_device->Release() : 0;
            g_esp += 4 + 4;
            break;
        }
        case 3: { // TestCooperativeLevel -> purge 4
            g_eax = s_host_d3d9_device ? static_cast<uint32_t>(s_host_d3d9_device->TestCooperativeLevel()) : 0;
            g_esp += 4 + 4;
            break;
        }
        case 16: { // Reset(this, pPresentationParameters) -> purge 8
            uint32_t pPP = MEM32(g_esp + 8);
            if (s_host_d3d9_device && pPP && g_mem_base) {
                D3DPRESENT_PARAMETERS* pp = reinterpret_cast<D3DPRESENT_PARAMETERS*>(ADDR(pPP));
                void* native_win = platform::Platform::instance().native_window_handle();
                pp->hDeviceWindow = reinterpret_cast<HWND>(native_win);
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->Reset(pp));
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 8;
            break;
        }
        case 17: { // Present(this, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion) -> purge 20
            if (s_host_d3d9_device) {
                void* native_win = platform::Platform::instance().native_window_handle();
                s_host_d3d9_device->Present(nullptr, nullptr, reinterpret_cast<HWND>(native_win), nullptr);
            }
            // Route through project renderer pacer to maintain 60 FPS swapchain pacing
            pcrecomp::renderer::Renderer::instance().present();
            g_eax = 0; // D3D_OK
            g_esp += 4 + 20;
            break;
        }
        case 41: { // BeginScene(this) -> purge 4
            if (s_host_d3d9_device) {
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->BeginScene());
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 4;
            break;
        }
        case 42: { // EndScene(this) -> purge 4
            if (s_host_d3d9_device) {
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->EndScene());
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 4;
            break;
        }
        case 43: { // Clear(this, Count, pRects, Flags, Color, Z, Stencil) -> purge 28
            uint32_t count = MEM32(g_esp + 8);
            uint32_t pRects = MEM32(g_esp + 12);
            uint32_t flags = MEM32(g_esp + 16);
            uint32_t color = MEM32(g_esp + 20);
            float z = *reinterpret_cast<float*>(ADDR(g_esp + 24));
            uint32_t stencil = MEM32(g_esp + 28);

            if (s_host_d3d9_device) {
                const D3DRECT* rects = (pRects && count > 0 && g_mem_base) ? reinterpret_cast<const D3DRECT*>(ADDR(pRects)) : nullptr;
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->Clear(count, rects, flags, color, z, stencil));
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 28;
            break;
        }
        case 44: { // SetTransform(this, State, pMatrix) -> purge 12
            uint32_t state = MEM32(g_esp + 8);
            uint32_t pMatrix = MEM32(g_esp + 12);
            if (s_host_d3d9_device && pMatrix && g_mem_base) {
                const D3DMATRIX* mat = reinterpret_cast<const D3DMATRIX*>(ADDR(pMatrix));
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->SetTransform(static_cast<D3DTRANSFORMSTATETYPE>(state), mat));
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 12;
            break;
        }
        case 47: { // SetViewport(this, pViewport) -> purge 8
            uint32_t pVp = MEM32(g_esp + 8);
            if (s_host_d3d9_device && pVp && g_mem_base) {
                const D3DVIEWPORT9* vp = reinterpret_cast<const D3DVIEWPORT9*>(ADDR(pVp));
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->SetViewport(vp));
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 8;
            break;
        }
        case 57: { // SetRenderState(this, State, Value) -> purge 12
            uint32_t state = MEM32(g_esp + 8);
            uint32_t val = MEM32(g_esp + 12);
            if (s_host_d3d9_device) {
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(state), val));
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 12;
            break;
        }
        case 58: { // GetRenderState(this, State, pValue) -> purge 12
            uint32_t state = MEM32(g_esp + 8);
            uint32_t pVal = MEM32(g_esp + 12);
            if (s_host_d3d9_device && pVal && g_mem_base) {
                DWORD* val = reinterpret_cast<DWORD*>(ADDR(pVal));
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->GetRenderState(static_cast<D3DRENDERSTATETYPE>(state), val));
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 12;
            break;
        }
        case 65: { // SetTexture(this, Stage, pTexture) -> purge 12
            uint32_t stage = MEM32(g_esp + 8);
            if (s_host_d3d9_device) {
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->SetTexture(stage, nullptr));
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 12;
            break;
        }
        case 81: { // DrawPrimitive(this, PrimitiveType, StartVertex, PrimitiveCount) -> purge 16
            uint32_t primType = MEM32(g_esp + 8);
            uint32_t startVert = MEM32(g_esp + 12);
            uint32_t primCount = MEM32(g_esp + 16);
            if (s_host_d3d9_device) {
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->DrawPrimitive(
                    static_cast<D3DPRIMITIVETYPE>(primType), startVert, primCount
                ));
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 16;
            break;
        }
        case 82: { // DrawIndexedPrimitive(this, Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount) -> purge 28
            uint32_t type = MEM32(g_esp + 8);
            int baseVert = static_cast<int>(MEM32(g_esp + 12));
            uint32_t minIdx = MEM32(g_esp + 16);
            uint32_t numVerts = MEM32(g_esp + 20);
            uint32_t startIdx = MEM32(g_esp + 24);
            uint32_t primCount = MEM32(g_esp + 28);
            if (s_host_d3d9_device) {
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->DrawIndexedPrimitive(
                    static_cast<D3DPRIMITIVETYPE>(type), baseVert, minIdx, numVerts, startIdx, primCount
                ));
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 28;
            break;
        }
        case 83: { // DrawPrimitiveUP(this, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride) -> purge 20
            uint32_t primType = MEM32(g_esp + 8);
            uint32_t primCount = MEM32(g_esp + 12);
            uint32_t pData = MEM32(g_esp + 16);
            uint32_t stride = MEM32(g_esp + 20);
            if (s_host_d3d9_device && pData && g_mem_base) {
                const void* verts = reinterpret_cast<const void*>(ADDR(pData));
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->DrawPrimitiveUP(
                    static_cast<D3DPRIMITIVETYPE>(primType), primCount, verts, stride
                ));
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 20;
            break;
        }
        case 84: { // DrawIndexedPrimitiveUP(this, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride) -> purge 36
            uint32_t primType = MEM32(g_esp + 8);
            uint32_t minVertIdx = MEM32(g_esp + 12);
            uint32_t numVerts = MEM32(g_esp + 16);
            uint32_t primCount = MEM32(g_esp + 20);
            uint32_t pIndexData = MEM32(g_esp + 24);
            uint32_t idxFmt = MEM32(g_esp + 28);
            uint32_t pVertData = MEM32(g_esp + 32);
            uint32_t stride = MEM32(g_esp + 36);

            if (s_host_d3d9_device && pIndexData && pVertData && g_mem_base) {
                const void* indices = reinterpret_cast<const void*>(ADDR(pIndexData));
                const void* verts = reinterpret_cast<const void*>(ADDR(pVertData));
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->DrawIndexedPrimitiveUP(
                    static_cast<D3DPRIMITIVETYPE>(primType), minVertIdx, numVerts, primCount,
                    indices, static_cast<D3DFORMAT>(idxFmt), verts, stride
                ));
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 36;
            break;
        }
        case 89: { // SetFVF(this, FVF) -> purge 8
            uint32_t fvf = MEM32(g_esp + 8);
            if (s_host_d3d9_device) {
                g_eax = static_cast<uint32_t>(s_host_d3d9_device->SetFVF(fvf));
            } else {
                g_eax = 0;
            }
            g_esp += 4 + 8;
            break;
        }
        default: {
            // Default handler: return D3D_OK and purge standard stdcall arguments
            g_eax = 0;
            g_esp += 4;
            break;
        }
    }
}

} // namespace pcrecomp::win32
