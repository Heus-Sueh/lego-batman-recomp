# Investigation: 0013 - Advancing Boot Gate G7 (Game Initialization Frontier)

> Documentation review (2026-09-19): **HISTORICAL**. Retain input initialization, ABI and CFG leads. The copied binary hash, historical labels and next-target list require revalidation against the selected executable.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## Revision
- **Executable**: `gta_sa.exe` (v1.0 US Clean / Unpacked)
- **SHA-256**: `8c609f10255a9b9a4c000a6e8b4e7a8e24484b901a7d65377f0f6706e4a28292`
- **Boot Gate**: G7 — Game Initialization (`CGame::Initialise`)
- **Milestone**: M8 — Engine Initialization

## Execution Frontier
- **Last successful VA**: `0x00619B60` (`RsEventHandler` / `rsINIT`)
- **Current VA**: `0x0053EC10` (`RsEventHandler` / `CGame` event handler)
- **Failure**: Discovery of new authentic loading screen and game initialization frontier calls.
- **Target VA**: `0x005900B0`, `0x00619410`, `0x00590860`, `0x00590990`, `0x00541DD0`

## Classification
- [x] Recompiler (lifter instruction, CFG, jump table, calling convention, flags, x87)
- [x] Runtime (guest memory, PE loading, Win32, dispatch, callback, threading, timing)
- [ ] Game-specific (middleware behavior, RenderWare driver, title-specific logic)

## Evidence
1. `Functions runtime reached` increased from 1 (1.0%) to 37 (35.9%) across 103 registered functions.
2. `Total function dispatches` grew from 1 to 693 dispatches.
3. Authentic sequences executed successfully:
   - `0x00538860` (`InitGameDirectory`), `0x00538900` (`FileOpen`), `0x005387D0` (`SetGameDirectory`)
   - `0x00530500` (`CControllerConfigManager_Reinitialise`), `0x0052FD70` (`ClearSettingsAssociation`), `0x0052F5F0` (`ReorderKeys`)
   - `0x00530640` (`InitDefaults`), `0x00530490` (`SetKeyAssociatedWithAction`)
   - `0x007469A0` (`InitialiseKeyboard`), `0x007485C0` (`InitialiseMouseAndJoypad`)
   - `0x0053F2D0` (`Frontend_Init`), `0x0052F6F0`, `0x0052F590`
   - `0x00619B60` (`RsEventHandler` handling `rsINIT` - event `0x15`)
   - `0x005902B0` (`CLoadingScreen_DisplaySplashWindow`)
   - `0x00574630` (`CMenuManager::LoadAllTextures`)
4. Unresolved call frontier identified:
   - `0x005900B0` (`CLoadingScreen` helper from `WinMain`)
   - `0x00619410` (`RsCameraShowRaster` from `CLoadingScreen_DisplaySplashWindow`)
   - `0x00590860` (`CLoadingScreen_DoProgress` from `WinMain`)
   - `0x00590990` (`CLoadingScreen_Continue` from `WinMain`)
   - `0x0058FF10` (`CLoadingScreen_Shutdown` calling `0x00731850`, `0x00731E90`, `0x00731CD0`)
   - `0x00541DD0` (`CGame::Initialise` sub-system) calling `0x0053F3C0`, `0x00746A10`, `0x0052DD90`, `0x00531140`, `0x005314A0`

## Hypothesis
- The game stalled previously before `rsINIT` due to two primary blockers:
  1. `RsPlatform.pDirectInput` was pre-initialized to a non-null pointer (`0x00C8D100`) before `WinMain`, triggering `0x007487EB: jne 0x74880a` which immediately branched to the error exit / shutdown routine.
  2. `0x00530640` encountered a HOODLUM crack trampoline at `0x00530645` jumping to unlifted address `0x00409ABF`, which obfuscated `push 0x3fb` (key binding 4).
  3. Missing CRT functions: `_strupr` (`0x00836F1E`), `_fsopen` (`0x008232D8`), and user directory helper (`0x00744FB0`).
  4. Jump table in `0x0052FD70` (`0x0052FE00`) needed structured switch lifting.

## Smallest Experiment
- Implemented `bridge_DirectInput8Create`, `bridge_crt_strupr`, `bridge_crt_fsopen`, and `bridge_func_00744FB0`.
- Added de-obfuscation rule for `0x00530640` and switch lifting for `0x0052FD70`.
- Initialized `RsPlatform.pDirectInput` as `0` so `DirectInput8Create` dynamically writes the COM pointer.
- Added `0x0052FD70`, `0x0052F5F0`, and `0x00530490` to `functions.toml`.
- Linked `recomp_trace_enter` with `extern "C"` to enable live function coverage tracking.

## Result
- Recompilation built cleanly with 0 errors across both `linux-amd64-release` and `linux-amd64-debug`.
- Functions reached jumped from 1 to 37 (35.9%), dispatches reached 693.
- All controller configurations, input devices, loading splash screen, and `rsINIT` completed naturally.

## Root Cause
- Premature direct memory injection of `pDirectInput` bypassed the DirectInput8Create COM initialization state machine.
- SecuROM crack trampolines required linear de-obfuscation in the lifter.

## Fix
- Configured dynamic `DirectInput8Create` bridge and corrected initial `pDirectInput` state in `run_lift.py`.
- Lifted authentic controller configuration methods (`0x0052FD70`, `0x0052F5F0`, `0x00530490`).
- Bound CRT string and file operations directly to VFS-backed portable runtime shims.

## Regression Test
- `just frontier`
- `out/build/linux-amd64-release/gta_sa --frames 5 --headless --coverage`
- Verified clean process termination and zero unresolved ICALLs during boot sequence.

## Upstream Candidate
- [x] Yes: candidate for upstream `pcrecomp` contribution (generic `DirectInput8Create` COM bridge, `_strupr`, and `_fsopen` file handle routing)
- [ ] No: project-specific revision manifest or configuration

## Next Frontier
- **New Last Successful VA**: `0x00619B60`
- **Next Blocker / Target**: `0x005900B0`, `0x00619410`, `0x00590860`, `0x00590990`, `0x00541DD0`
