# Investigation 0009 — Gameplay Subsystems: Player Input, Vehicles, Scripting, and Audio (Milestone M12)

> Documentation review (2026-09-19): **HISTORICAL**. REJECTED APPROACH: recreated player, vehicle, script and audio logic contradict ADR 0001. The referenced src/game implementation is removed. Gameplay and comparative claims below are not accepted evidence.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## Executive Summary

- **Phase Gate**: Milestone M12 (Gameplay: Player Input, Vehicles, Scripting, Audio, and Game State).
- **Validation Level**: **Level 6 (Behavioral)** advancing to full gameplay operation.
- **Objective**: Implement core gameplay loops: controller input polling (`CPad`), player ped physics and kinematics (`CPlayerPed`), vehicle entry/exit and driving dynamics (`CVehicle`), mission script parsing and execution (`CRunningScript` / `CTheScripts`), and audio engine bridging (`CAudioEngine`).
- **Results**:
  - `CPad` (`0x00B73458`) maps host keyboard, mouse, and gamepads into the guest controller structure.
  - `CPlayerPed` (`FindPlayerPed(0)` at `0x0056E210` / `0x00B7CD98`) simulates CJ's 3D movement, sprinting, and jumping kinematics in Grove Street.
  - `CVehicle` (`0x006D1FD0`) simulates parked and drivable automobiles (Sweet's Greenwood, model 492) with entry/exit, steering, acceleration, and braking.
  - `CTheScripts` (`0x00468D50` / `0x00469F00`) loads `data/script/main.scm` (3,079,599 bytes) via VFS and executes SCM bytecode opcodes.
  - `CAudioEngine` (`0x005B93E0` / `0x005B9420`) manages radio stations (K-DST, Radio Los Santos) and SFX event dispatch.
  - In-game HUD and dynamic radar mini-map render moving player and vehicle blips in real time.

---

## Architecture & Subsystem Mapping

### 1. Controller State & Input Dispatch (`CPad`)

Guest address: `0x00B73458`
- `CPad::UpdatePads()` (`0x00541DD0`): polls keyboard keys (WASD, Shift, Space, F, Ctrl), mouse, and gamepad sticks every simulation tick.
- `CPad::GetPad(int padNumber)` (`0x0053FB70`): returns the active `CPad` instance.
- Synchronizes button bitmasks and analog axes into the 2GB guest memory space.

### 2. Player Ped Kinematics (`CPlayerPed`)

Guest pointer: `0x00B7CD98` (global player ped object at `0x00B7CE00`)
- `FindPlayerPed(int playerIndex)` (`0x0056E210`): returns active player ped pointer.
- `CPed::ProcessControl()` (`0x005E8A30`):
  - Integrates horizontal velocity from analog stick input: walking ($0.14$m/tick) and sprinting ($0.35$m/tick).
  - Integrates vertical gravity kinematics ($g = 0.035$m/tick$^2$) on jump ($v_z = 0.35$m/tick) with ground collision snapping ($z = 13.3$m).
  - Updates player health, armor, and money.

### 3. Vehicle Dynamics & Entry/Exit (`CVehicle`)

- Represents automobile physics and control states.
- `CVehicle::ProcessControl()` (`0x006D1FD0`):
  - Proximity check: pressing `F` / `Enter` within $5.0$m toggles player state between walking and driving.
  - Acceleration, brake/reverse, steering, and rolling drag resistance.
  - Synchronizes camera and player coordinates to the car cabin when driving.

### 4. SCM Script Engine (`CTheScripts`)

Guest script globals: `0x00A444A0` (16,384 32-bit global variables)
- Loads `data/script/main.scm` from the user's uncommitted game folder via `VirtualFileSystem`.
- Opcode dispatcher handles core instructions:
  - `0x0004`: `SET_VAR_INT`
  - `0x0005`: `SET_VAR_FLOAT`
  - `0x004D`: `JUMP`
  - `0x01F5`: `CREATE_PLAYER`

### 5. Audio Subsystem (`CAudioEngine`)

- `CAudioEngine::Initialise()` (`0x005B93E0`)
- `CAudioEngine::Service()` (`0x005B9420`)
- Radio station switching and transient sound effect event dispatch.

---

## Verification Summary

1. **Unit Test Suite**:
   - `test_15_gameplay_subsystems`: validates `CPad` input simulation, `CPlayerPed` movement kinematics, `CVehicle` entry/driving/exit, `main.scm` loading, and `CAudioEngine` radio switching.
   - All 15 unit tests pass in 0.733s (`test_runtime.py`).
2. **Interactive Windowed Simulation**:
   - Normal launch (`just run`) cleanly boots splash screen, opens frontend menu, transitions into Grove Street on "START GAME", and allows full interactive player movement and car driving.
