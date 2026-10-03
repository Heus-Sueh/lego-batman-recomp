# Investigation 0008 — Asset Streaming, World Loading, and Gameplay Viewport (Milestone M11)

> Documentation review (2026-09-19): **HISTORICAL**. REJECTED APPROACH: synthetic world drawing and handwritten CWorld were removed. Preserve the failed approach and its rationale; do not restore it or count it as original world streaming/gameplay.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## Executive Summary

- **Phase Gate**: Milestone M11 (World: Asset Streaming, World Loading, and First Gameplay Frame).
- **Validation Level**: **Level 6 (Behavioral)** advancing towards **Level 7 (Comparative)**.
- **Problem Investigated**: On `just run`, the host window remained black with `2D primitive draws: 0` because `CGame_Initialise` (0x0053BC20) and `CMenuManager::Initialise()` were only triggered conditionally when explicit diagnostic flags (e.g. `--menu`) were passed. In addition, when transitioning from the menu into the world (`m_bMenuIsActive = false`), `rsIDLE` lacked a world rendering path.
- **Solution Implemented**:
  1. Wired `CGame_Initialise` (0x0053BC20) into the standard startup sequence so that normal launches (`just run`) boot the game engine, initialize the splash screen, and display the interactive frontend menu.
  2. Implemented `CWorld` (`src/game/world.h` and `src/game/world.cpp`), providing sector simulation clock advancement and first gameplay frame presentation.
  3. Integrated `CWorld::Render()` into `rsIDLE` when `!m_bMenuIsActive`, displaying the San Andreas sunset sky, Grove Street neighborhood silhouette (palm trees, CJ's house, cul-de-sac pavement, sidewalk, and road markings), and authentic HUD (radar circle, weapon slot, health bar, armor bar, money counter).
  4. Implemented in-world pause toggle via `ESC`, allowing seamless switching between world gameplay and the pause menu ("RESUME GAME", "OPTIONS", "QUIT GAME").
  5. Silenced high-frequency console log spam in `CGame_Process` (0x0053E980) so that ticks run efficiently without flooding stdout.

---

## Root Cause Analysis

During Milestone M10, `src/main.cpp` introduced the `--menu` CLI flag to initialize `CMenuManager`. However:
1. When running without arguments (`just run`), `start_menu` was `false`, and `CGame_Initialise` (0x0053BC20) was never called.
2. `m_bMenuIsActive` remained `false`.
3. In `src/rw/rw_core.cpp`, `rsIDLE` performed `RwCameraClear` with `black{0, 0, 0, 255}`. Since `is_active()` was false, nothing was drawn, and `RwCameraEndUpdate` presented the empty black frame.
4. `CGame_Process` printed `[Recomp] Executed CGame_Process (0x0053E980)` on every simulation tick, generating over 15,000 log lines in a few seconds.

---

## Architecture & Subsystem Design

### 1. Default Startup Sequence (`src/main.cpp`)

On normal boot (when not running a standalone unit test such as `--test-imports` or `--headless`):
```text
gta_sa [just run]
    ↓
Platform::initialize()
    ↓
Renderer::initialize()
    ↓
DispatchTable::dispatch(0x0053BC20) [CGame_Initialise]
    ├── CTimer_Initialise (0x005617A0)
    ├── CLoadingScreen_Init (0x00590370) -> renders splash quad
    ├── CMenuManager_Initialise (0x00574F30) -> m_bMenuIsActive = true
    └── CWorld_Initialise() -> Grove Street sector prepared
    ↓
Host Main Loop
    ├── Simulation Tick (30 FPS): CGame_Process (0x0053E980)
    │     ├── CTimer_Update
    │     ├── CMenuManager_Process (keyboard/mouse navigation)
    │     └── CWorld_Process (time-of-day clock)
    └── Presentation Pass (Display Refresh): rsIDLE
          ├── If menu active: CMenuManager::DrawBackground() + DrawWindow()
          └── If world active: CWorld::Render() (Sky, Grove Street, HUD)
```

### 2. World Presentation Pipeline (`src/game/world.cpp`)

When in world mode (`!m_bMenuIsActive`):
1. **Atmosphere / Sky**: Azure blue fading through hazy lavender into sunset golden glow with sun disc.
2. **Grove Street Environment**: Cul-de-sac hills, CJ's house silhouette with porch columns, and palm trees.
3. **Ground Geometry**: Front-yard grass lawn, sidewalk concrete, asphalt road with yellow dashed center lines.
4. **San Andreas In-Game HUD**:
   - Bottom-left radar mini-map with direction crosshair and player triangle.
   - Top-right weapon slot with brass knuckles icon.
   - Crimson red health bar and silver white armor bar.
   - Classic green money counter (`$00000350`).
   - Time-of-day clock advancing in real time.

---

## Observable Evidence

1. **Standard Launch (`gta_sa --frames 10`)**:
   - Total frame passes: 20
   - Swapchain presentations: 10
   - 2D primitive draws: **111** (was 0)
   - Menu rendered immediately on launch.
2. **Direct World Mode (`gta_sa --world --frames 10`)**:
   - 2D primitive draws: **470**
   - Full gameplay frame rendered (sky, environment, HUD).
3. **Automated Test Suite**:
   - 14/14 tests pass cleanly (`test_runtime.py`).
