# Capability Index: Recompilation Skills

This document provides a quick reference index for the 7 static recompilation skills, mapping common development tasks to their responsible skills and demonstrating how skills compose to resolve complex problems. It is a routing document, not evidence that any milestone or test has passed.

---

## 1. Task to Skill Mapping

| Development Task | Responsible Skill | Key Output |
|---|---|---|
| Analyze PE headers, sections, exports, or imports | [`$binary-analysis`](../.agents/skills/binary-analysis/SKILL.md) | PE metadata, memory mapping plan |
| Disassemble raw bytes, delineate basic blocks | [`$binary-analysis`](../.agents/skills/binary-analysis/SKILL.md) | Disassembly listing, CFG candidates |
| Determine calling convention (`cdecl`, `stdcall`, `thiscall`) | [`$binary-analysis`](../.agents/skills/binary-analysis/SKILL.md) | Verified convention & stack purge count |
| Lift x86 instructions to structured C/C++ AST | [`$static-recomp`](../.agents/skills/static-recomp/SKILL.md) | C++ source in `generated/recomp/` |
| Recover jump tables and generate structured C++ switches | [`$static-recomp`](../.agents/skills/static-recomp/SKILL.md) | Structured switch blocks |
| Configure preimage-checked deterministic patches | [`$static-recomp`](../.agents/skills/static-recomp/SKILL.md) | Entries in `patches.toml` |
| Triage crashes, segfaults, or CPU execution halts | [`$recomp-debug`](../.agents/skills/recomp-debug/SKILL.md) | 3-Failure classification, root cause |
| Resolve unlifted indirect calls (`ICALL`) at runtime | [`$recomp-debug`](../.agents/skills/recomp-debug/SKILL.md) | Discovery, bridge, or lifted target |
| Triage stack imbalances or corrupted return addresses | [`$recomp-debug`](../.agents/skills/recomp-debug/SKILL.md) | Callee purge mismatch identified |
| Evaluate progress on the 7-level validation ladder | [`$recomp-validation`](../.agents/skills/recomp-validation/SKILL.md) | Declared validation level (L1–L7) |
| Enforce evidence tags (`VERIFIED`, `INFERRED`) | [`$recomp-validation`](../.agents/skills/recomp-validation/SKILL.md) | Tagged assertions, negative controls |
| Create isolated regression tests for recompiler/runtime | [`$recomp-validation`](../.agents/skills/recomp-validation/SKILL.md) | Automated tests in `tests/` |
| Implement host presentation pacing & swapchain pacing | [`$native-renderer`](../.agents/skills/native-renderer/SKILL.md) | Decoupled swapchain presentation |
| Advance through 5-tier renderer migration (Tier 0 to 4) | [`$native-renderer`](../.agents/skills/native-renderer/SKILL.md) | Progressive graphics modernization |
| Intercept textures, rasters, and draw command streams | [`$native-renderer`](../.agents/skills/native-renderer/SKILL.md) | Modern host GPU resources |
| Implement SDL window creation, event loop, and input | [`$platform-port`](../.agents/skills/platform-port/SKILL.md) | Platform layer in `src/platform/` |
| Translate Win32 APIs (`kernel32`, `user32`, `dinput8`, `dsound`)| [`$platform-port`](../.agents/skills/platform-port/SKILL.md) | Clean C++ shims in `src/win32/` |
| Coordinate multi-stage project milestones | [`$recomp-project`](../.agents/skills/recomp-project/SKILL.md) | Active milestone gate status |

---

## 2. Skill Composition Examples

In practice, resolving a complex blocker requires chaining specialized skills without duplicating their work:

### Scenario A: Resolving an Unresolved ICALL
```text
1. [recomp-debug]      Detect and log unresolved ICALL at guest VA (0x00531220).
       ↓
2. [binary-analysis]   Inspect target VA disassembly, identify prologue/epilogue, verify calling convention and purge.
       ↓
3. [static-recomp]     Add entry to functions.toml, run lifter to generate C++, recompile.
       ↓
4. [recomp-validation] Re-run execution trace, verify that execution advances past 0x00531220 (Level 3/4 validation).
```

### Scenario B: Adding a Native Host Input Device
```text
1. [platform-port]     Implement SDL_Gamepad polling in src/platform/ and map to DirectInput COM interface in src/win32/.
       ↓
2. [recomp-validation] Create negative control test ensuring unplugged/remapped controllers are detected.
       ↓
3. [recomp-project]    Update milestone gate progress towards interactive gameplay (M9).
```

### Scenario C: Addressing Display Stutter on High-Refresh Displays
```text
1. [recomp-debug]      Profile frame times with Tracy; discover mismatch between 30 FPS guest ticks and 144 Hz display refresh.
       ↓
2. [native-renderer]   Implement Tier 1 host swapchain pacing queue decoupled from guest simulation ticks.
       ↓
3. [recomp-validation] Capture latency statistics; verify p95/p99 frame presentation variance is within sub-millisecond tolerances.
```
