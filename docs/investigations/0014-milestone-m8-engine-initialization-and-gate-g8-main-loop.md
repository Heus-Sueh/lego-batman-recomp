# Investigation 0014: Milestone M8 (Engine Initialization) and Boot Gate G8 (Main Loop)

> Documentation review (2026-09-19): **HISTORICAL**. Retain register/initialization observations. Old M8/M9 labels and 23-test counts are historical. Register-restoration workarounds need root-cause evidence, not automatic reuse.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## Historical reported status: Solved (not current gate validation)

## Summary
Advanced the static recompilation pipeline across **Milestone M8 (Engine Initialization)**, **Milestone M9 (Renderer Presentation)**, and **Boot Gate G8 / G9 (Main Loop & First Frame Presentation)**.

Lifting the authentic loading screen, DirectInput pad polling, and controller configuration subsystems unblocked the authentic execution path branching from `WinMain_v10US (0x00748710)`. The binary now successfully navigates the entire initialization sequence, enters the authoritative RenderWare message pump and idle loop, executes simulation ticks through authentic `CGame::Process` (`0x0053C680`), presents native swapchain frames to the host platform presenter, and shuts down with clean status (exit code 0).

Runtime coverage surged to **55 functions runtime reached (47.0%)** across **1046 function dispatches**, with all **23 unit tests** passing cleanly.

## Execution Frontier State
- **Phase Gate**: Milestone M8 (Engine Initialization) / Milestone M9 (Renderer)
- **Boot Gate**: Boot Gate G8 (Main Loop) / Boot Gate G9 (First Frame)
- **Previous Reached Functions**: 37 functions (31.6%), 693 dispatches
- **New Reached Functions**: 55 functions (47.0%), 1046 dispatches
- **Swapchain Presentations**: 1+ native frames presented
- **Validation Ladder**: Level 5 (Visual / Swapchain Presentation)

## Evidence & Investigation Steps

### 1. Unblocking Pad & Controller Subsystems
- Integrated and lifted authentic GTA:SA functions:
  - `0x0053F3C0` (`CPad_UpdatePads`, 368 B, thiscall)
  - `0x00746A10` (`InitialisePads`, 880 B, cdecl)
  - `0x0052DD90` (`CControllerConfigManager_ClearSettings`, 32 B, thiscall)
  - `0x00531140` (`CControllerConfigManager_SaveSettings`, 864 B, thiscall)
  - `0x005314A0` (`CControllerConfigManager_LoadSettings`, 464 B, thiscall)
  - `0x00746060` (`IsAppActive`, 16 B, cdecl)
  - `0x00747CD0` (`CheckShiftKeys`, 224 B, cdecl)
  - `0x0052DDB0` (`CControllerConfigManager_GetIsKeyboardKeyDown`, 1456 B, thiscall)
- Implemented `bridge_import_GetKeyState` for IAT entry `0x008582B0` (`USER32.dll!GetKeyState`) used by `CheckShiftKeys`.
- Initialized `RsPlatform.hWnd` (`0x00C8CF88`) and `RsPlatform.pDirectInput` (`0x00C8CF88 + 0x18`) to eliminate null pointer indirection during DirectInput keyboard device creation.

### 2. Resolving Loading Screen & TXD Store Bridges
- Implemented bridges for RenderWare video mode query (`0x007F2CF0`, `RwEngineGetVideoModeInfo`) and keyboard event dispatcher (`0x00619560`, `RsKeyboardEventHandler`).
- Implemented `CTxdStore` slot and reference tracking bridges:
  - `0x00731C80` (`CTxdStore::AddTxdSlot`)
  - `0x007320B0` (`CTxdStore::LoadTxd`)
  - `0x00731A00` (`CTxdStore::AddRef`)
  - `0x007316A0` (`CTxdStore::PushCurrentTxd`)
  - `0x007319C0` (`CTxdStore::SetCurrentTxd`)
  - `0x007316B0` (`CTxdStore::PopCurrentTxd`)
- Connected CRT random number generation (`0x00821B11` `srand`, `0x00821B1E` `rand`) and `sprintf` (`0x00821BB5`) for loading screen progress messages.

### 3. Fixing WinMain Frame Pointer (EBP) Epilogue Corruption
- In `WinMain_v10US (0x00748710)`, four consecutive accessibility key restoration calls at `0x00748EC0`, `0x00748EC8`, `0x00748ED0`, `0x00748ED8` execute `call ebp` expecting `ebp` to contain `SystemParametersInfoA` (`MEM32(0x0085826C)`).
- Because `ebp` was previously modified by preceding function frame setups, `ebp` contained a guest stack address (`0x002FFDE0`), triggering an unresolved ICALL crash.
- Inserted deterministic register restoration `ebp = MEM32(0x85826C);` preceding `0x00748EC0`, ensuring clean `SystemParametersInfoA` accessibility restoration upon shutdown.

### 4. Enabling Authentic Main Loop & Frame Pacing
- Connected `0x00438450` (`CPad::ResetPads`), `0x00541D70` (`CPad::Clear`), `0x00561AA0` (`CTimer::Resume`), `0x0053C550` (`CGame::InitialiseCore1`), and `0x0053C680` (`CGame::Process`).
- Initialized `RwEngineInstance` (`0x00C97B24`) to point to `RwGlobals` (`0x00C9BF00`) and mapped device render state helper (`0x00C9BF20`).
- Decoupled `g_frame_counter` in `shim_PeekMessageA` from Windows message pump polling loops, ensuring frames are incremented solely upon authentic presentation ticks.

## Verification
- **Unit Tests**:
  ```bash
  just test
  # Ran 23 tests in 0.482s. OK.
  ```
- **Release Build & Execution**:
  ```bash
  cmake --build --preset linux-amd64-release --parallel
  out/build/linux-amd64-release/gta_sa --frames 10 --headless --coverage
  # Functions registered: 117
  # Functions runtime reached: 55 (47.0%)
  # Total function dispatches: 1046
  # Swapchain presentations: 1
  # Simulation ticks: 1
  # Recomp runtime completed cleanly.
  ```
- **Debug Build**:
  ```bash
  cmake --build --preset linux-amd64-debug --parallel
  # Build succeeded with 0 errors.
  ```
