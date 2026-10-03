---
name: native-renderer
description: "Implement and advance incremental renderer migration across 5 tiers (Tier 0 passthrough to Tier 4 native Vulkan), decouple guest simulation from host presentation pacing, intercept textures/geometry, and manage host swapchains."
---

# native-renderer: Renderer Migration & Host Presentation Pacing

## 1. Purpose
Manage the graphics presentation lifecycle and progressive renderer modernization in static recompilation. Execute the 5-tier renderer migration roadmap, decouple guest simulation tick rates from host display presentation, manage host swapchains, and intercept graphics resources.

## 2. When to Use
- When implementing or configuring host display presentation and swapchain pacing.
- When diagnosing frame pacing jitter, stutter, or vsync tearing.
- When advancing graphics capability across the 5 migration tiers.
- When intercepting textures, shaders, vertex buffers, or draw commands.
- Do NOT use for general Win32 window event handling or input devices (use `$platform-port`).
- Do NOT use for profiling non-rendering bottlenecks (use `$recomp-debug`).

## 3. Inputs
- Host window context (from `$platform-port` / SDL).
- Graphics API boundary targets (Direct3D 9 / OpenGL / Vulkan).
- Active renderer migration tier (Tier 0 through Tier 4).
- Display characteristics (target refresh rate, vsync mode, aspect ratio constraints).

## 4. Workflow

### Step 1: The 5-Tier Renderer Migration Roadmap
Advance graphics incrementally without attempting a full rewrite prematurely:

```text
Tier 0: Original Semantics
   ↓
Tier 1: Native Presentation & Host Frame Pacing
   ↓
Tier 2: Resource Interception (Textures, Buffers, Render Targets)
   ↓
Tier 3: Draw-Command Interception (Geometry pipelines)
   ↓
Tier 4: Pure Native Backend (Vulkan / Modern DXGI)
```

1. **Tier 0 (Original Semantics)**: Preserve original graphics API calls (e.g. Direct3D 9). On Linux, operate via translation layers (e.g. DXVK). Proves that engine device creation and clear operations succeed.
2. **Tier 1 (Native Presentation & Pacing)**: The project creates the native host window and swapchain. Intercept presentation at the end-of-frame boundary:
   - Present the backbuffer to the host swapchain with aspect-ratio letterboxing/pillarboxing.
   - Enforce host frame pacing decoupled from guest simulation ticks.
3. **Tier 2 (Resource Interception)**: Intercept texture creation, pixel format conversions, and vertex buffer uploads. Mirror guest resources directly in host GPU memory.
4. **Tier 3 (Draw-Command Interception)**: Intercept geometry draw calls (primitive submission, index lists) before they reach the legacy driver pipeline.
5. **Tier 4 (Pure Native Backend)**: Replace all legacy driver calls with a native Vulkan rendering pipeline.

### Step 2: Simulation vs Presentation Decoupling
To eliminate display judder while preserving authentic simulation physics:
1. **Never uncap the guest simulation tick rate** if the game logic relies on a fixed timestep.
2. Maintain guest simulation at its verified fixed timestep; do not infer a
   target rate from a different title or renderer implementation.
3. Run the host swapchain presentation loop at the native display refresh rate (60 Hz, 120 Hz, 144 Hz, 240 Hz, VRR).
4. Use monotonic high-resolution clocks (`std::chrono::steady_clock`) to schedule present calls with microsecond precision.

### Step 3: Aspect Ratio & Viewport Correction
1. Preserve target game aspect ratio (e.g., 4:3 or 16:9) on arbitrary display resolutions.
2. Apply centered viewport letterboxing (black bars top/bottom) or pillarboxing (black bars left/right) without stretching the internal UI.

## 5. Evidence Rules
- Measure visual performance using frame presentation times, median, p95, and p99 latency spikes, never just a raw average FPS counter.
- Confirm visual correctness by capturing rendered framebuffers at Level 5+ validation.

## 6. Validation
- Run rendering frame capture and confirm image integrity against expected reference screens.
- Profile presentation pacing with Tracy or GPU frame capture tools (RenderDoc).

## 7. Failure Handling
- If host presentation stutters: check if the simulation tick rate is drifting, or if the presentation loop is blocking on guest mutexes.
- If graphics corrupt or display black screens: verify texture format conversion flags (e.g. ARGB8888 vs RGBA8888) and depth/stencil format support.

## 8. Anti-Patterns
- **Premature Tier 4 Rewrite**: Attempting a complete Vulkan backend before Tier 0/Tier 1 displays the first valid frame.
- **Uncapped Simulation Hacks**: Injecting wall-clock delta times into guest simulation loops, breaking physics and script timers.
- **UI Distortion**: Stretching 4:3 viewports to widescreen displays without aspect ratio correction.

## 9. Knowledge Capture
When renderer work completes:
- Record verified display modes, texture formats, and pacing parameters in `docs/architecture/`.
- If an engine-specific shader or rendering quirk was discovered, document it in `docs/gotchas/`.

## 10. Completion Criteria
- Current renderer tier is functioning and verified by frame output.
- Frame pacing is stable with sub-millisecond jitter on host displays.
- Aspect ratio scaling is correct across multiple resolutions.
