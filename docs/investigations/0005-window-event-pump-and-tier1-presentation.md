# Investigation 0005: Window Event Pump & Tier 1 Native Presentation

> Documentation review (2026-09-19): **HISTORICAL**. Retain window/event and pacing experiments. Historic host-loop behavior and frame counts do not establish current guest timing or gameplay correctness.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## Overview

- **Investigation Date**: 2026-09-13
- **Phase Gate**: Milestone M7 (Window & Event Processing) & Milestone M8 (Engine Initialization & Tier 1 Presentation)
- **Objective**: Establish SDL3 native windowing with hardware presentation, full input tracking (Win32 `VK_*`, relative mouse, gamepad), decoupled simulation vs presentation frame pacing, RenderWare camera lifecycle integration, and multi-frame host execution.

---

## 1. Native Window & Hardware Presenter (`Platform`)

The host platform boundary in `src/platform/platform_sdl.cpp` coordinates native windowing and presentation:
- **Hardware-Accelerated Presenter**: Created via `SDL_CreateRenderer(window, nullptr)`, enabling Tier 1 native swapchain presentation across Linux and Windows.
- **Aspect Ratio Preservation**: Employs `SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX)`, guaranteeing that 4:3 or 16:9 gameplay projections scale cleanly with letterboxing/pillarboxing on arbitrary display resolutions without stretching game geometry or UI coordinates.
- **V-Sync Synchronization**: Controlled through `SDL_SetRenderVSync(renderer, vsync ? 1 : 0)`.

```text
Host Display / Swapchain
         ▲
         │ (SDL_RenderPresent)
   Platform Layer (src/platform/)
         ▲
         │ (Tier 1 begin_frame / end_frame / present)
   Renderer Subsystem (src/renderer/)
         ▲
         │ (RwCameraBeginUpdate / RwCameraClear / RwCameraEndUpdate)
   RenderWare Subsystems (src/rw/)
         ▲
         │ (RsEventHandler: rsIDLE)
   Recompiled Game Loop (WinMain -> CGame_Process)
```

---

## 2. Decoupled Frame Pacing Architecture

Grand Theft Auto: San Andreas was designed around a calibrated 30 FPS simulation clock (`CTimer::m_snTimeInMilliseconds` and `CTimer::ms_fTimeStep`). Presenting directly at 30 FPS on high-refresh displays (60 Hz, 120 Hz, 144 Hz) produces noticeable judder, while uncapping simulation speed breaks game physics and script timers.

In accordance with project architecture, simulation pacing is decoupled from host presentation:
1. **Host Clock**: Monotonic microsecond timestamping using `std::chrono::steady_clock`.
2. **Tick Duration**: $\Delta t_{\text{sim}} = \frac{1,000,000}{\text{target\_fps}} \approx 33,333\ \mu\text{s}$ for 30 FPS.
3. **Pacing Accumulator**:
   - `consume_simulation_ticks()` accumulates real elapsed time between frames.
   - For every elapsed $\Delta t_{\text{sim}}$, one discrete simulation step (`CGame_Process` / 0x0053E980) executes.
   - Simulation tick count is clamped to 4 steps maximum per presentation pass to eliminate spiral-of-death stalls during window drag or background pauses.
4. **Presentation Cadence**: Presentation occurs on every host event loop pass, allowing presentation to run at host display refresh rate.

---

## 3. Input Subsystem & Scancode Translation

Input tracking operates across three device modalities:
1. **Keyboard**:
   - SDL3 scancodes are mapped to Win32 Virtual-Key codes (`VK_*`) via `sdl_scancode_to_win32_vk()` (e.g. `SDL_SCANCODE_W` $\to$ `'W'`, `SDL_SCANCODE_SPACE` $\to$ `VK_SPACE (0x20)`, `SDL_SCANCODE_RETURN` $\to$ `VK_RETURN (0x0D)`).
   - Fast state lookup: `Platform::is_key_down(vk_code)` and `Platform::is_scancode_down(scancode)`.
2. **Mouse**:
   - Tracks cursor position $(x, y)$, mouse buttons (Left, Middle, Right), and relative deltas $(\Delta x, \Delta y)$ for first-person/third-person camera look.
   - `Platform::set_relative_mouse_mode(bool enabled)` wraps `SDL_SetWindowRelativeMouseMode`.
3. **Gamepad**:
   - Hot-plugging supported via `SDL_EVENT_GAMEPAD_ADDED` and `SDL_EVENT_GAMEPAD_REMOVED`.
   - Native button and analog axis querying via `SDL_GetGamepadButton` and `SDL_GetGamepadAxis`.

---

## 4. RenderWare Engine Hooks & Camera Lifecycle

RenderWare camera functions are cataloged in `config/revisions/gta_sa_compact/functions.toml` and wired into `src/rw/rw_core.cpp`:
- `RwCameraBeginUpdate (0x007EE130)`: Initiates backbuffer frame pass via `Renderer::begin_frame()`.
- `RwCameraClear (0x007EE1D0)`: Clears viewport and backbuffer color/depth.
- `RwCameraEndUpdate (0x007EE140)`: Finalizes frame and triggers swapchain presentation via `Renderer::present()`.
- `RsEventHandler (0x00619B70)`: Dispatches `rsINIT` (subsystem setup), `rsRWINIT` (device setup), and `rsIDLE` (simulation tick + camera update cycle).

---

## 5. Verification & Test Suite

The automated test suite in `tests/test_runtime.py` validates all Milestone M7 & M8 features:
- `test_06_headless_multi_frame_loop`: Runs 10 engine simulation frames in headless mode, verifying that 10 simulation ticks and 10 swapchain presentations complete cleanly.
- `test_07_windowed_presentation_pacing`: Initializes an 800x600 native SDL3 window, runs 5 frames of the event pump and presentation loop, and verifies clean shutdown.
- `test_08_custom_resolution_and_fps`: Verifies configurable simulation tick rate (`--fps 60` resulting in a $16,666\ \mu\text{s}$ tick duration) and custom window geometry.

```bash
$ just test
".venv/bin/python3" -m unittest discover -s tests -p "test_*.py"
........
----------------------------------------------------------------------
Ran 8 tests in 0.182s

OK
```
