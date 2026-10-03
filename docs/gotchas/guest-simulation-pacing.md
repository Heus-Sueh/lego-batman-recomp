# Gotchas: Fixed Guest Simulation & Pacing Pitfalls

> Review 2026-09-19: PROJECT POLICY AND UNVERIFIED BEHAVIORAL CONTEXT. The 30 FPS policy is normative; detailed subsystem effects below need original/recompiled comparative evidence. High headless frame counts do not measure real-time simulation or establish fidelity.

## The 30 FPS Invariant

The game engine of *Grand Theft Auto: San Andreas* was designed around a nominal **30 FPS fixed simulation tick rate** (governed by its internal frame limiter). 

Many core subsystems have simulation constants hardcoded to this step duration ($\Delta t \approx 33.33\text{ ms}$):
- **Vehicle Physics**: Wheel suspension damping, collision resolution, and angular momentum integration behave erratically if ticked at arbitrary frequencies.
- **Swimming & Diving**: Swimming stamina meters drain proportionally to simulation frames; running at uncapped frame rates causes CJ to drown almost instantly.
- **Script Engine & Mission Logic**: Mission scripts frequently rely on frame delays (`WAIT 0`, `WAIT 10`) rather than wall-clock timers. Changing the simulation rate alters mission pacing, cutscene synchronization, and AI decision loops.
- **Audio & Radio Synchronization**: Radio track transitions and speech streaming timers are tied to simulation ticks.

## Common Anti-Patterns

### Anti-Pattern 1: Naive Delta-Time Injection
Attempting to replace the fixed timestep by injecting wall-clock `dt` into guest physics routines causes instability, clipping through world terrain, and mission script failures.

### Anti-Pattern 2: Uncapped Simulation Loop
Running the original main loop as fast as the GPU can render generates severe physics glitches and breaks animations.

## The Solution: Decoupled Simulation & Presentation

Instead of modifying guest simulation code:
1. **Keep the guest simulation ticking** at its calibrated 30 FPS rate using high-resolution monotonic sleep timers.
2. **Decouple the host presentation loop**: Present the resulting frames to the host display (60 Hz, 120 Hz, 144 Hz, 240 Hz, or VRR) with host-side frame pacing and interpolation.
3. Measure presentation latency and compare guest behavior before claiming reduced judder or behavioral fidelity.
