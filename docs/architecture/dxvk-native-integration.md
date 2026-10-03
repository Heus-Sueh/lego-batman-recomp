# DXVK-Native Integration Architecture (Tier 0 Direct3D 9 over Vulkan)

> Review 2026-09-19: PROPOSAL WITH PARTIAL IMPLEMENTATION. Library loading and guest thunks exist; full original 3D rendering, parity and pacing are not established by those facts. The progression table is not the canonical M0–M9 gate model. See [runtime architecture](../runtime-architecture.md).

## 1. Executive Summary & Objective

This document defines the architectural specification for integrating **DXVK-Native** into the GTA: San Andreas static recompilation project as the **Tier 0 Hardware Passthrough** renderer.

```text
+-------------------------------------------------------------------------+
|                Lifted Authenticated Game Logic (x86-32)                 |
|            (CLoadingScreen, CMenuManager, CFont, CWorld, etc.)          |
+-------------------------------------------------------------------------+
                                   |
                                   v
+-------------------------------------------------------------------------+
|                  RenderWare Graphics Engine (v3.6/3.7)                  |
|          (RwEngine, RwDevice, RwCamera, RpWorld, RwD3D9Device)          |
+-------------------------------------------------------------------------+
                                   | (32-bit guest COM calls)
                                   v
+-------------------------------------------------------------------------+
|               Guest-to-Host COM Thunking Layer (src/win32/)             |
|       - 32-bit handle dispatch table in lower 2GB guest memory           |
|       - Translation of 32-bit guest VTables to 64-bit host methods      |
+-------------------------------------------------------------------------+
                                   | (64-bit host Direct3D 9 COM API)
                                   v
+-------------------------------------------------------------------------+
|                   DXVK-Native (libdxvk_d3d9.so)                         |
|             - Direct3D 9 to Vulkan 1.3/1.4 translation layer            |
|             - WSI Driver: SDL3 (native Linux Wayland/X11 surface)       |
+-------------------------------------------------------------------------+
                                   |
                                   v
+-------------------------------------------------------------------------+
|                    Host GPU Driver (Vulkan / RADV / AMD)                |
+-------------------------------------------------------------------------+
```

DXVK-Native enables the project to run the authentic RenderWare Direct3D 9 backend without Wine or Windows emulation, bridging directly to native Linux Vulkan through the host SDL3 platform layer.

---

## 2. Motivation & Strategic Alignment (ADR 0001)

Per **[ADR 0001: Recompilation vs Recreation](../decisions/0001-recompilation-vs-recreation.md)**, authentic x86 machine instructions lifted from `gta_sa.exe` must be preserved. Hand-written C++ reimplementations of game subsystems under `src/game/` are strictly prohibited.

The original PC executable routes all 3D rendering, materials, skinning, and lighting through RenderWare's Direct3D 9 driver (`d3d9.dll`). While the project currently operates on **Tier 2 (2D Interception & Monotonic Presentation Pacing)** via SDL3 geometry rendering, reaching full 3D gameplay (peds, vehicles, world geometry, weather shaders) requires either:
1. Re-implementing every single RenderWare pipeline in modern Vulkan (hundreds of shaders and state machines), or
2. **Tier 0 Passthrough via DXVK-Native**: Providing a native Direct3D 9 COM implementation backed by Vulkan, allowing lifted RenderWare driver code to execute authentically.

Tier 0 is a proposed route toward reusing the original graphics behavior. Correct COM marshalling, complete exercised API coverage and comparative rendering evidence are still required; loading DXVK does not establish parity.

---

## 3. Core Architectural Challenges & Solutions

### 3.1 32-bit Guest vs 64-bit Host Pointer Thunking

The recompiled executable targets Linux x86_64 (`linux-amd64-release`). However, the lifted RenderWare code operates in 32-bit guest address space (`0x00400000 - 0x01500000`).
- RenderWare's global device pointer (`RwD3D9Device` at guest VA `0x00C97C20`) is a 32-bit pointer to an `IDirect3DDevice9` COM vtable.
- DXVK-Native produces 64-bit host COM objects (`IDirect3DDevice9*` whose pointers are 64 bits wide and point to host heap addresses > 4GB).

#### Solution: Handle Table & Guest COM Wrapper
We construct a guest-accessible VTable inside guest memory (under 2GB, typically in the reserved range `0x00C97000 - 0x00C97FFF`):

```text
Guest Memory (32-bit VA)                        Host Memory (64-bit VA)
+-----------------------+                       +-----------------------------+
| 0x00C97C20:           |                       | DXVK-Native IDirect3DDevice9|
|   RwD3D9Device        |                       |   Host COM Object           |
|       |               |                       |   (vtable pointers, state)  |
|       v               |                       +-----------------------------+
| 0x00C97000:           |                                      ^
|   Guest VTable:       |                                      |
|   [0] QueryInterface  |                                      |
|   [1] AddRef          |                                      |
|   [2] Release         |                                      |
|   ...                 |                                      |
|   [41] SetRenderState | ---> shim_D3D9Device_SetRenderState -+
|   [42] GetRenderState | ---> shim_D3D9Device_GetRenderState -+
|   [81] DrawPrimitive  | ---> shim_D3D9Device_DrawPrimitive  -+
+-----------------------+
```

Every guest call `call dword ptr [eax + offset]` invokes a thunk function that:
1. Extracts guest arguments from the guest stack (`esp + 4`, `esp + 8`, etc.).
2. Translates any guest buffer pointers or resource handles to their underlying host DXVK objects.
3. Invokes the real 64-bit method on `IDirect3DDevice9`.
4. Writes the `HRESULT` return code into guest `eax`.
5. Cleans up the stack according to `stdcall` callee-cleanup purge rules (`ret N`).

### 3.2 Window System Integration (WSI) via SDL3

Upstream DXVK-Native officially provides an SDL3 WSI backend (`DXVK_WSI_DRIVER=SDL3`).
- When `Direct3DCreate9()` is called, DXVK initializes its Vulkan instance and device.
- In `IDirect3D9::CreateDevice()`, DXVK queries the native window handle.
- By configuring DXVK to use SDL3 WSI, DXVK creates the `VkSurfaceKHR` directly from our active `SDL_Window*` (managed by `src/platform/platform_sdl.cpp`), automatically handling Wayland and X11 surfaces on Linux.

### 3.3 Decoupled Presentation Pacing (ADR 0002)

Even when Direct3D 9 is active:
- RenderWare calls `IDirect3DDevice9::Present()` inside `RwCameraEndUpdate()`.
- The presentation must pass through our monotonic host pacing loop (`Renderer::present()`) to enforce smooth 60 FPS swapchain pacing and prevent unthrottled spinlooping.
- Presentation throttling is handled via `sleep_until` aligned to 16.6ms intervals without perturbing the fixed 30 FPS guest simulation tick rate.

---

## 4. Migration Progression Matrix

| Tier | Name | Graphics Driver | Scope | Status |
|---|---|---|---|---|
| **Tier 0** | DXVK-Native Passthrough | Vulkan (via `libdxvk_d3d9.so`) | Full authentic RenderWare 3D world, lighting, materials, vehicles, peds | **Specification Defined (M10)** |
| **Tier 1** | Presentation Decoupled | Host Monotonic Swapchain | Frame pacing, judder elimination, 60/144 Hz display sync | **Implemented paths; validation scope must be checked** |
| **Tier 2** | 2D Geometry & Rasters | SDL3 GPU / Hardware RenderGeometry | Splash screens, fonts (`CFont`), HUD, 2D sprites (`CSprite2d`) | **Implemented paths; validation scope must be checked** |
| **Tier 3** | 3D Geometry Interception | Modern Shader Pipelines | Camera matrices, `RwIm3D`, custom post-processing | Planned |
| **Tier 4** | Native Vulkan Architecture | Pure Vulkan (No D3D9) | Modern physically-based rendering, raytracing, ultrawide | Long-term target |

---

## 5. Implementation Action Plan

1. **Submodule Pinning**:
   - Add `thirdparty/dxvk-native` as a Git submodule tracking the upstream stable release.
2. **Build Configuration**:
   - Integrate DXVK-Native build into CMake using Meson external project or pre-built library dependency.
3. **Thunk Layer Implementation**:
   - Implement `src/win32/win32_d3d9.h` and `src/win32/win32_d3d9.cpp` containing the 32-to-64-bit COM thunk table for `IDirect3D9` and `IDirect3DDevice9`.
4. **Vulkan Validation**:
   - Verify on local hardware: a supported Vulkan device and SDL window backend; record the actual driver/tool versions with the result.
