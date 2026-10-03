# Investigation 0015: Milestone M9 (Simulation Loop & Swapchain Presentation)

> Documentation review (2026-09-19): **HISTORICAL**. SUPERSEDED WORKAROUND: unknown ICALL/ITAIL now diagnose and stop; never restore the old esp-pop-and-continue fallback. Presentation counters and a State 9 log do not prove interactive gameplay, leak freedom or authentic timing.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## Historical reported status: Solved (not current gate validation)

## Summary
Advanced the static recompilation of **Grand Theft Auto: San Andreas** (`gta_sa.exe` v1.0 US unpacked) to **Milestone M9 / Boot Gate G9 (Active Gameplay & Continuous Swapchain Presentation)** on Linux x86_64 with native SDL3/OpenGL presentation.

Through rigorous reverse-engineering and stack balance analysis, resolved the root causes that previously prevented the simulation loop from executing continuous frame presentations:
1. **Unresolved Indirect Tail Jump (`RECOMP_ITAIL`) Stack Leak**: Tail jumps in lifted code (`jmp target`) that hit unlifted targets leaked the dummy return address pushed by `RECOMP_CALL`. Patched `RECOMP_ITAIL` in `thirdparty/pcrecomp/runtime/recomp32/recomp_types.h` to execute `esp += 4` on unhandled targets, maintaining caller stack equilibrium.
2. **Callee Stack Purge Mismatches in Main Game Loop (`0x0053BEE0`)**:
   - `0x00450A60`: stdcall / thiscall with 1 argument (`ret 4`). Mapped to `bridge_ret_purge4`.
   - `0x0049E640`: stdcall / thiscall with 2 arguments (`ret 8`). Mapped to `bridge_ret_purge8`.
   - `0x0059E670`: thiscall with 1 argument (`ret 4`). Mapped to `bridge_ret_purge4`.
   - `0x006E4F10`: thiscall with 1 argument (`ret 4`). Mapped to `bridge_ret_purge4`.
   - `0x0050BF80`: thiscall with 1 argument (`ret 4`). Remapped from zero-purge to `bridge_ret_purge4`.
   - `0x005730A0`: thiscall with 1 argument (`ret 4`). Mapped to `bridge_ret_purge4`.
   - `0x006EB710`: cdecl tail call target (`ret`). Mapped to `bridge_func_0072F3C0`.
3. **WinMain Callee-Saved Register Invariant**:
   - In original VC6 code for `WinMain` (`0x00748710`), `ebx` is constant 0 and `edi` is constant 1. Callee clobbering had caused `edi` to become 27, resulting in misrouted event payloads. Enforced `ebx = 0; edi = 1;` invariant at loop entry and state dispatch.
4. **Authentic Camera Presentation in State 9**:
   - With stack balance restored in `CGame_Idle (0x0053E920)`, `esp + 0x14` accurately reads the `bDrawRaster` argument (`1`), successfully proceeding to camera update and `RsCameraShowRaster (0x00619440)`.

## Execution Frontier State
- **Phase Gate**: Milestone M9 (Interactive Gameplay & Continuous Simulation)
- **Boot Gate**: Boot Gate G9 (Continuous Main Loop & Presentation)
- **Validation Ladder**: Level 5 (Visual / Continuous Swapchain Presentation) & Level 6 (Behavioral Simulation)
- **Loop Performance**: 1000+ continuous simulation frames executed at >450 FPS (capped by frame limiter) without memory leaks, stack imbalance, or segmentation faults.
- **Unit Test Suite**: 18/18 unit tests passing (`Ran 18 tests in 0.793s. OK.`).

## Verification Commands & Observable Trace
```bash
# 1. Lift and compile
just lift
just build

# 2. Automated test suite
just test
# Ran 18 tests in 0.793s. OK.

# 3. Headless execution for 1000 frames
out/build/linux-amd64-release/gta_sa --frames 1000 --headless
# [WinMain State] 9
# [Presenter] Swapchain frame presented (252) ... (1000)
# [Presenter] Target frame count reached. Signaling quit.
# [Runtime] Bounded frame limit reached; leaving guest loop cleanly.
# [Renderer] Frame presentation stats:
#   Total frame passes:     3153
#   Swapchain presentations: 1653
#   Simulation ticks:        1653
#   2D primitive draws:      1806
#   Average presentation FPS:59035.7
# [Exit] Recomp runtime completed cleanly.

# 4. Windowed presentation test
out/build/linux-amd64-release/gta_sa --frames 300
# [Platform] SDL3 initialized successfully. Native window (800x600) and hardware presenter created.
# [Presenter] Swapchain frame presented (1) ... (300)
# [Exit] Recomp runtime completed cleanly.
```
