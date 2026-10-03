# ADR 0002: Decoupled Simulation and Host Presentation Pacing

## Context
Original GTA: San Andreas runs on a 30 FPS fixed simulation tick rate. Running this engine on modern displays (60 Hz, 120 Hz, 144 Hz, 240 Hz, VRR) presents a conflict:
- If the simulation loop is uncapped to run at display refresh rate, vehicle physics, swimming stamina, mission wait timers, and script execution glitch or fail.
- If host presentation is locked to 30 FPS using naive swapchain vsync on a 60 Hz or 144 Hz display, severe visual judder and frame pacing unevenness occur.

## Decision
We decouple the **guest simulation tick rate** from the **host presentation loop**:
1. Guest simulation continues ticking at its calibrated 30 FPS rate governed by monotonic high-resolution timers.
2. Host presentation runs independently at the native display refresh rate, sampling completed simulation frames and presenting them through host-controlled swapchains with sub-millisecond pacing.

## Consequences
- **Positive**: Intended outcome: smoother presentation while retaining guest timing policy. Measure frame pacing and compare guest behavior before claiming that this outcome is achieved.
- **Negative**: Requires careful swapchain synchronization and presentation queue management in `src/platform/` and `src/renderer/`.
- **Rejected Alternative**: Injecting variable delta-time (`dt`) into guest routines was rejected because GTA SA physics and script logic are fundamentally non-linear with respect to timestep variations.
