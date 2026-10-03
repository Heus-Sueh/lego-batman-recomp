# Investigation 0006: Renderer Tier 2 Resource Interception & 2D Presentation

> Documentation review (2026-09-19): **HISTORICAL**. Retain raster and Im2D bring-up details. Historic renderer tiers and visual claims need current capture evidence; old M9 is not today's interactive-gameplay gate.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## Overview

- **Investigation Date**: 2026-09-13
- **Phase Gate**: Milestone M9 (Renderer: Graphics Device Initialisation, Tier 2 Resource Interception & First Presentation)
- **Validation Level**: **Level 5 — Visual** (Splash screen / 2D textured geometry rendered to native host swapchain)
- **Objective**: Intercept RenderWare `RwRaster` and `RwTexture` lifecycles, establish the 2D Immediate Mode rendering pipeline (`RwIm2D`), and integrate the game splash presentation system (`CLoadingScreen`).

---

## 1. Tier 2 Resource Interception: Rasters & GPU Textures

RenderWare manages 2D and 3D surface memory through `RwRaster` structures, wrapped in `RwTexture` containers. In legacy Direct3D 9, rasters directly managed `IDirect3DTexture9` or surface handles.

In our Tier 2 architecture (`src/renderer/renderer.cpp` and `src/rw/rw_core.cpp`):
1. **Raster Memory**: `RwRasterCreate()` allocates CPU-accessible pixel memory (`cpPixels`) matching the requested dimensions and 32-bit ARGB pitch.
2. **Lock / Unlock Lifecycle**:
   - `RwRasterLock()` grants guest code a raw byte pointer to pixel rows.
   - `RwRasterUnlock()` signals completion of CPU modification.
3. **GPU Texture Upload Bridge**:
   - `Renderer::on_raster_unlocked()` synchronizes modified raster pixels directly with a host GPU texture (`SDL_Texture`) via `SDL_UpdateTexture()`.
   - Rasters and GPU textures are automatically tracked in `m_raster_textures`, ensuring zero GPU resource leakage upon `RwRasterDestroy()`.

```text
RwRasterCreate(w, h, depth, flags)
       │
       ▼
RwRasterLock()  ──>  CPU writes pixels (loadscs / sprites)
       │
       ▼
RwRasterUnlock()
       │
       ▼
Renderer::on_raster_unlocked()
       │ (SDL_UpdateTexture)
       ▼
Host GPU Texture (SDL_Texture)
```

---

## 2. 2D Immediate Mode Geometry Submission (`RwIm2D`)

GTA: San Andreas uses RenderWare 2D immediate-mode primitives (`RwIm2D`) for all user interface components: loading splashes, menu backgrounds, text glyphs, HUD bars, radar, and faders.

### Vertex & Topology Mapping

`RwIm2DVertex` defines 2D vertices in screen-space $(x, y, z, \text{rhw})$ with 32-bit ARGB color and normalized texture coordinates $(u, v)$:
- **Color Unpacking**: ARGB 32-bit integer is unpacked to linear floating-point components $(r, g, b, a) \in [0.0, 1.0]$.
- **Topology Triangulation**:
  - `rwPRIMTYPETRIFAN`: Vertices are triangulated using fan indexing: $(0, i, i+1)$ for $i \in [1, N-2]$.
  - `rwPRIMTYPETRISTRIP`: Vertices are triangulated with alternating winding: $(i, i+1, i+2)$ for even $i$, and $(i+1, i, i+2)$ for odd $i$.
  - `rwPRIMTYPETRILIST`: Rendered directly or mapped from a 16-bit guest index buffer.
- **Hardware Submission**: Executed through `SDL_RenderGeometry()` with the active `SDL_Texture*` bound.

---

## 3. GTA Loading & Splash Screen Integration (`CLoadingScreen`)

The game splash presentation pipeline was discovered and cataloged in `config/revisions/gta_sa_compact/functions.toml`:
- `CLoadingScreen_Init (0x00590370)`: Main entry point called from `CGame_Initialise`.
- `CLoadingScreen_LoadSplashes (0x00590B10)`: Prepares the splash raster and creates the `loadscs` texture.
- `CLoadingScreen_RenderSplashWindow (0x00590480)`: Emits a fullscreen 2D quad and submits it to the presentation pipeline.
- `CLoadingScreen_Shutdown (0x00590410)`: Cleans up textures upon game initialization completion.

During recompiled execution from `mainCRTStartup` $\to$ `WinMain` $\to$ `CGame_Initialise`, the splash screen is initialized and rendered to the host display swapchain, officially achieving **Level 5 — Visual** on the project validation ladder.

---

## 4. Verification & Automated Test Results

The test suite in `tests/test_runtime.py` verifies all Milestone M9 features:
- `test_09_raster_resource_lifecycle`: Verifies `RwRasterCreate`, pixel locking, CPU pixel manipulation, `RwRasterUnlock`, and clean memory deallocation via `RwTextureDestroy`.
- `test_10_splash_screen_presentation`: Verifies splash texture creation, textured quad submission, and multi-frame presentation to the native SDL3 window.
- `test_05_recompiled_startup_dispatch`: Confirms that `CLoadingScreen_Init` executes during the recompiled `mainCRTStartup` startup sequence and attains Level 5 Visual status.

```bash
$ just test
".venv/bin/python3" -m unittest discover -s tests -p "test_*.py"
..........
----------------------------------------------------------------------
Ran 10 tests in 0.403s

OK
```
