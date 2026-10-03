# Audit of Agents, Skills, Rules, and Project Knowledge

> Review 2026-09-19: HISTORICAL AUDIT. Inventory, test counts and next-frontier claims below describe earlier snapshots. Use [skill-index](skill-index.md), [runtime architecture](runtime-architecture.md) and `just frontier` for active work. This review does not finalize the pre-existing deletion set.

> Scope correction (2026-09-17): this audit covers the current seven skills in
> `.agents/skills/`. The legacy skill names described below are historical
> working-tree entries, not active skills. No files were moved or deleted by
> this audit.

Date: 2026-09-17  
Scope: Repository agent structure, skills (`.agents/skills/`), project instructions (`AGENTS.md`), and technical documentation (`docs/`).

---

## 1. Inventory of Files

### 1.1 Root & Configuration Files
- `AGENTS.md` (203 lines, 12.7 KB): Project instructions, core invariants, non-negotiable architecture, skill routing table, validation ladder, milestone definitions, boot gates, priority system, and definition of done.
- `README.md` (305 lines, 13.6 KB): High-level project README, build instructions, toolchain prerequisites.
- `justfile` (94 lines, 3.5 KB): Command runner recipes (`build`, `frontier`, `trace`, `lift`, `disasm`, etc.).

### 1.2 Skills (`.agents/skills/`)
1. **`binary-analysis`**, **`static-recomp`**, **`recomp-debug`**, **`recomp-validation`**, **`native-renderer`**, **`platform-port`**, and **`recomp-project`** are the active focused skills. Each has the required ten-section structure and valid frontmatter.

2. **Legacy entries**
   - `SKILL.md` (53 lines, 4.8 KB): `pcrecomp` pipeline diagram, 7-step workflow, invariants (recomp vs recreation, clean binary, function states, revision integrity).
   - `references/manifest-and-ownership.md` (58 lines, 2.9 KB): `config/revisions/` schema, deterministic patches, generated code boundaries.
   - `references/runtime-function-discovery.md` (52 lines, 2.3 KB): Unresolved ICALL dispatch, stub policy, discovery protocol.
   - `references/workflow.md` (55 lines, 2.7 KB): Step-by-step pipeline execution (`pe_analyze.py`, `disasm32.py`, `run_lift.py`).
   - `agents/openai.yaml` (11 lines, 372 B): Agent metadata definition.

3. The old names (`pcrecomp-project`, `recomp-performance-debugging`, `sdl-platform-portability`, `x86-pe-recomp-re`, and the monolithic bring-up skill) are retained only in the working-tree deletion set and must not be treated as active inventory until deletion is explicitly finalized.
   - `SKILL.md` (34 lines, 2.7 KB): Measurement loop, baseline capture, decoupling simulation & presentation, bottleneck localization, guardrails.
   - `references/debugging-evidence.md` (41 lines, 1.9 KB): Triage for crashes, stack imbalances, and Win32 boundary errors.
   - `references/measurement-and-triage.md` (41 lines, 1.9 KB): Pacing vs frame count, Tracy instrumentation, latency statistics.
   - `agents/openai.yaml` (11 lines, 372 B): Agent metadata definition.

4. **`sdl-platform-portability`**
   - `SKILL.md` (48 lines, 3.2 KB): Host architecture, platform layer, Win32 translation, 5-tier renderer migration summary, pacing decoupling, review rules.
   - `references/build-and-runtime.md` (46 lines, 2.3 KB): CMake presets, toolchain versions, packaging layout.
   - `references/cross-platform-verification.md` (34 lines, 1.6 KB): Testing guidelines on Linux x86_64 and Windows x86_64.
   - `references/renderware-migration-tiers.md` (45 lines, 2.4 KB): Detailed 5 tiers (Tier 0 to Tier 4).
   - `agents/openai.yaml` (11 lines, 372 B): Agent metadata definition.

5. **`x86-pe-recomp-re`**
   - `SKILL.md` (38 lines, 3.1 KB): Investigation loop (failure context, calling convention, guest memory model, boundary identification, hypothesis, validation).
   - `references/calling-conventions-and-abi.md` (58 lines, 2.9 KB): x86-32 calling conventions (`cdecl`, `stdcall`, `thiscall`, `fastcall`), stack purge evidence, `GuestAddress` translation.
   - `references/investigation-playbook.md` (56 lines, 3.1 KB): Crash and hang diagnosis, root-cause identification.
   - `references/renderware-and-win32-subsystems.md` (50 lines, 3.0 KB): RenderWare 3.6/3.7 lifecycle (`RwEngine`, `RwDevice`, `RwCamera`, `RpWorld`) and Win32 shims (`user32`, `dinput8`, `dsound`, `d3d9`).
   - `agents/openai.yaml` (11 lines, 372 B): Agent metadata definition.

### 1.3 Documentation (`docs/`)
- `docs/RECURRING_ACTIONS.md` (113 lines, 5.7 KB): Frontier target classification table (CRT, Win32, DRM crack trampoline, switch table, game logic), Recipe A (adding to `functions.toml`), Recipe B (CRT bridge), Recipe C (crack trampoline de-obfuscation), Recipe D (switch table lifting).
- `docs/future.md` (1140 lines, 17.9 KB): Detailed roadmap for porting static recompilation and GTA SA to Android ARM64.
- `docs/investigations/`:
  - `TEMPLATE.md` (34 lines, 1.6 KB): Standard investigation template.
  - `0001-executable-identification-and-securom.md` (82 lines, 3.7 KB)
  - `0002-startup-callgraph-and-subsystem-frontiers.md` (110 lines, 4.8 KB)
  - `0003-code-generation-and-dispatch-integration.md` (105 lines, 4.7 KB)
  - `0004-runtime-guest-memory-and-import-resolution.md` (86 lines, 3.8 KB)
  - `0005-window-event-pump-and-tier1-presentation.md` (124 lines, 5.3 KB)
  - `0006-renderer-tier2-raster-interception-and-im2d.md` (102 lines, 4.6 KB)
  - `0007-frontend-menu-manager-and-input-navigation.md` (106 lines, 4.7 KB)
  - `0008-asset-streaming-and-world-loading.md` (105 lines, 4.6 KB)
  - `0009-gameplay-player-vehicles-scripting-and-audio.md` (88 lines, 3.9 KB)
  - `0010-renderware-3d-pipeline-and-world-geometry.md` (134 lines, 6.1 KB)
  - `0011-authentic-game-asset-loading-and-rendering.md` (112 lines, 5.0 KB)
  - `0012-developer-tooling-and-authentic-recompilation-pipeline.md` (203 lines, 9.4 KB)
  - `0013-advancing-boot-gate-g7-game-initialization-frontier.md` (110 lines, 4.9 KB)
  - `0014-milestone-m8-engine-initialization-and-gate-g8-main-loop.md` (108 lines, 4.9 KB)

---

## 2. Identified Duplications and Overlaps

| Topic / Concept | Occurrences in Current Structure | Assessment |
|---|---|---|
| **x86-32 Calling Conventions & ABI** | `pc-static-recomp-bringup/SKILL.md` (§14), `x86-pe-recomp-re/references/calling-conventions-and-abi.md`, `AGENTS.md` (§Guest Memory Model) | Duplicated. Belongs canonically in `binary-analysis` skill; summary rule in `AGENTS.md`. |
| **5-Tier Renderer Migration** | `sdl-platform-portability/references/renderware-migration-tiers.md`, `pc-static-recomp-bringup/references/target-selection-and-migration.md`, `AGENTS.md` (§RenderWare renderer migration) | Duplicated. Belongs canonically in `native-renderer` skill; architecture overview in `docs/architecture/`. |
| **Boot Gates (G0–G10)** | `pc-static-recomp-bringup/SKILL.md` (§9), `AGENTS.md` (§Boot Gates) | Duplicated verbatim. Belongs canonically in `recomp-project` and `recomp-validation`. |
| **Validation Ladder (L1–L7)** | `pc-static-recomp-bringup/references/validation-and-journal.md`, `AGENTS.md` (§Validation Ladder) | Duplicated verbatim. Belongs canonically in `recomp-validation`. |
| **The 3 Failure Classes & Golden Rule** | `pc-static-recomp-bringup/SKILL.md` (§2, §3), `AGENTS.md` (§Core Principles) | Duplicated. Belongs in `recomp-debug` skill and `AGENTS.md` core rules. |
| **Execution Frontier & Loop** | `pc-static-recomp-bringup/SKILL.md` (§6, §7), `x86-pe-recomp-re/SKILL.md`, `pcrecomp-project/references/runtime-function-discovery.md` | Overlapping. Execution frontier debugging belongs strictly in `recomp-debug`. |
| **Simulation vs Presentation Decoupling** | `recomp-performance-debugging/SKILL.md`, `sdl-platform-portability/SKILL.md`, `pc-static-recomp-bringup/references/target-selection-and-migration.md`, `AGENTS.md` | Fragmented. Presentation timing belongs in `native-renderer`, profiling in `recomp-debug`, game tick specifics in `docs/gotchas/`. |
| **Recompilation vs Recreation Invariant** | `AGENTS.md` (§Core Principles), `pcrecomp-project/SKILL.md` (§Invariants), `pc-static-recomp-bringup/SKILL.md` (§4) | Duplicated across multiple skills. Belongs in `AGENTS.md` as primary repository rule and `docs/decisions/0001-recompilation-vs-recreation.md`. |

---

## 3. Project-Specific Leaks in Generic Skills

The following items are title-specific to GTA: San Andreas or RenderWare, but currently reside inside skills that should be generic to any static recompilation:

1. **RenderWare Engine Specifics**:
   - `x86-pe-recomp-re/references/renderware-and-win32-subsystems.md` details `RwEngineInit`, `RwDeviceSystemFunction`, `RwCameraBeginUpdate`, `RpWorld`, `RpClump`, `RpAtomic`. This is specific to RenderWare-based games.
   - *Target*: Move to `docs/architecture/renderware-subsystems.md`.
2. **DirectInput / DirectSound / Win32 Implementation Notes**:
   - Mappings of GTA SA input to `SDL_Gamepad` and sound buffers are game-specific integration details.
   - *Target*: Move to `docs/architecture/win32-compatibility.md`.
3. **DRM / Hoodlum Crack Trampolines**:
   - Discussion of `.HOODLUM` crack section (`0x401000..0x409000`), formula `val1 + val2 + 1`, and SecuROM v7 `.text` decryption.
   - *Target*: Move to `docs/gotchas/drm-crack-trampolines.md` and `docs/research/`.
4. **GTA SA 30 FPS Tick Rate & Physics Traps**:
   - Hardcoded guest simulation dependencies (vehicle physics, swimming stamina, script wait opcodes).
   - *Target*: Move to `docs/gotchas/guest-simulation-pacing.md`.
5. **Concrete Addresses and Path Formats in Generic Skills**:
   - References to `gta_sa.exe`, `config/revisions/gta_sa_<sha>/`, `gta_sa_compact`, etc., in generic skills (`x86-pe-recomp-re`, `pcrecomp-project`).
   - *Target*: Skills should refer to abstract target binaries (`<target>.exe`) and generic config layouts; concrete paths remain in `AGENTS.md` and `docs/`.

---

## 4. Content Classification Table

Every piece of content is categorized into one of six categories:
- `PROCEDURE`: Reusable procedural method for agents -> Skill.
- `PROJECT_RULE`: Repository-specific constraint or invariant -> `AGENTS.md`.
- `PROJECT_KNOWLEDGE`: Discovered technical reality or subsystem detail -> `docs/`.
- `REFERENCE`: External project architecture, patterns, or baseline -> `docs/references/`.
- `OBSOLETE`: Proven outdated or invalid (none discarded without confirmation).
- `DUPLICATE`: Content duplicated across files -> Canonicalize and cross-reference.

| Content Source / Section | Classification | Proposed Destination | Rationale |
|---|---|---|---|
| **PE Analysis & Header Extraction** | `PROCEDURE` | `binary-analysis` skill | Generic procedure applicable to any PE32 binary. |
| **x86-32 Calling Convention Verification (`ret N`, `add esp, N`)** | `PROCEDURE` | `binary-analysis` skill | Fundamental reverse engineering procedure for x86-32. |
| **Function Discovery & Disassembly Workflow** | `PROCEDURE` | `binary-analysis` skill | Generic disassembler / CFG discovery steps. |
| **Targeted Instruction Lifting & C++ Codegen** | `PROCEDURE` | `static-recomp` skill | Core static recompilation generation workflow. |
| **Jump / Switch Table Recovery & C++ Mapping** | `PROCEDURE` | `static-recomp` skill | Reusable codegen technique for switch tables. |
| **Deterministic Preimage-Checked Patches** | `PROCEDURE` | `static-recomp` skill | Generic methodology for managing binary diffs. |
| **Execution Frontier Maintenance & Debugging** | `PROCEDURE` | `recomp-debug` skill | Core loop for advancing recomp execution. |
| **3-Failure Classification & Golden Rule** | `PROCEDURE` | `recomp-debug` skill | Triage protocol for root-causing defects. |
| **Unresolved ICALL Resolution Loop** | `PROCEDURE` | `recomp-debug` skill | Procedure for diagnosing indirect call dispatch faults. |
| **Stack Imbalance Localization** | `PROCEDURE` | `recomp-debug` skill | Systematic debugging of callee/caller purge mismatches. |
| **7-Level Validation Ladder** | `PROCEDURE` | `recomp-validation` skill | Canonical validation criteria for claims. |
| **Evidence Policy (`VERIFIED`, `INFERRED`, etc.)** | `PROCEDURE` | `recomp-validation` skill | Strict rules governing claim credibility. |
| **Negative Controls & Regression Tests** | `PROCEDURE` | `recomp-validation` skill | Verification techniques for validators. |
| **5-Tier Renderer Migration Protocol** | `PROCEDURE` | `native-renderer` skill | Step-by-step strategy for decoupling & porting rendering. |
| **Presentation Pacing & Tick Decoupling** | `PROCEDURE` | `native-renderer` skill | Generic swapchain pacing algorithm. |
| **Host Platform Abstraction & SDL Integration** | `PROCEDURE` | `platform-port` skill | Rules for clean host OS separation. |
| **Win32 Compatibility Shim Pattern** | `PROCEDURE` | `platform-port` skill | Guidelines for emulating Win32 on POSIX/Linux. |
| **Recomp Lifecycle Orchestration & Gate Routing** | `PROCEDURE` | `recomp-project` skill | Minimal coordinator directing multi-skill workflow. |
| **Recompilation vs Recreation Invariant** | `PROJECT_RULE` | `AGENTS.md` + `docs/decisions/` | Non-negotiable repository policy. |
| **No Copyrighted Retail Data in Git** | `PROJECT_RULE` | `AGENTS.md` | Non-negotiable legal/clean-room repository policy. |
| **Clean Binary / Unpacked Executable Requirement** | `PROJECT_RULE` | `AGENTS.md` + `docs/research/` | Binary prerequisite for analysis and lifting. |
| **`generated/recomp/` Edit Prohibition** | `PROJECT_RULE` | `AGENTS.md` | Non-negotiable codegen boundary rule. |
| **Milestone Gate Definitions (M0–M14)** | `PROJECT_RULE` | `AGENTS.md` + `recomp-project` | Repository progress milestones. |
| **English Language Requirement for All Artifacts** | `PROJECT_RULE` | `AGENTS.md` | Repository convention. |
| **RenderWare 3.6/3.7 Subsystem Architecture** | `PROJECT_KNOWLEDGE`| `docs/architecture/renderware-subsystems.md` | Specific to GTA SA RenderWare implementation. |
| **Win32 Translation Specifics for GTA SA** | `PROJECT_KNOWLEDGE`| `docs/architecture/win32-compatibility.md` | Concrete input/audio/windowing mapping details. |
| **Fast Decision Matrix & Recurring Recipes** | `PROJECT_KNOWLEDGE`| `docs/implementation/recurring-recipes.md` | Concrete GTA SA resolution recipes (CRT, DRM, etc.). |
| **SecuROM v7 Encryption & Crack Trampolines** | `PROJECT_KNOWLEDGE`| `docs/gotchas/drm-crack-trampolines.md` | Specific discovery regarding GTA SA executables. |
| **GTA SA 30 FPS Fixed Simulation Gotchas** | `PROJECT_KNOWLEDGE`| `docs/gotchas/guest-simulation-pacing.md` | Documenting game physics & timer traps. |
| **Stack Balance & ABI Traps** | `PROJECT_KNOWLEDGE`| `docs/gotchas/stack-balance-and-abi.md` | Documenting real traps encountered during bring-up. |
| **Android ARM64 Recompilation Roadmap** | `PROJECT_KNOWLEDGE`| `docs/architecture/android-portability-roadmap.md` | Long-term porting architecture from `future.md`. |
| **RexGlue Static Recompilation Reference** | `REFERENCE` | `docs/references/rexglue.md` | External reference project analysis. |
| **pcrecomp Upstream Baseline Reference** | `REFERENCE` | `docs/references/pcrecomp-upstream.md` | External toolbox upstream relationship. |
| **Investigations 0001–0014** | `PROJECT_KNOWLEDGE`| `docs/investigations/` (Kept intact) | Historical execution frontier investigations. |
| **Old inline ABI tables in `AGENTS.md`** | `DUPLICATE` | Point to `binary-analysis` skill | Redundant duplicate of binary analysis rules. |
| **Old inline renderer tiers in `AGENTS.md`** | `DUPLICATE` | Point to `native-renderer` skill | Redundant duplicate of renderer skill. |
| **Old boot gates duplicate in `pc-static-recomp-bringup`** | `DUPLICATE` | Point to `recomp-project` skill | Redundant duplicate of project milestones. |

---

## 5. Reorganization Status & Operational Next Steps

### 5.1 Reorganization Status: AUDITED / PARTIALLY NORMALIZED
The repository has an initial seven-skill split and an audit, but this audit found that the reorganization is not fully complete:
- [x] **Phase 1 (Tasks 1 & 2)**: Audit and classification completed in `docs/agent-audit.md`.
- [x] **Phase 2 (Tasks 8 & 11)**: Structured knowledge base established under `docs/` (`architecture/`, `implementation/`, `gotchas/`, `references/`, `decisions/`).
- [x] **Phase 3 (Tasks 3, 4, 5, 6, 7, 10, 12)**: 7 standardized skills exist under `.agents/skills/` following the uniform 10-section structure.
- [x] **Phase 4 (Tasks 9 & 13)**: `AGENTS.md` and the capability index use the normalized skill names; legacy skill deletions are present in the working tree.
- [ ] **Phase 5 (Tasks 14, 15, 16)**: Partially complete. Headless validation passes 16/18 tests; two windowed tests fail without an SDL video device. Runtime/codegen changes are present in the working tree, so a zero-diff claim is not valid.

### 5.3 Audit conclusions

- The seven current skills have distinct routing responsibilities and the required sections.
- Project-specific knowledge is still present in the skill descriptions for Win32/SDL and the renderer; this is acceptable only as routing context, while concrete addresses and discoveries remain in `docs/`.
- The old skills (`agentic-reverse-engineering`, `pcrecomp-project`, `recomp-performance-debugging`, `sdl-platform-portability`, and `x86-pe-recomp-re`) appear as deleted in the current worktree. Because this audit is non-destructive, their removal should be finalized only after confirming the deletion set is intentional and all unique references have been preserved.
- `README.md` previously overstated test success; it now records 16/18 headless-host results and the SDL display dependency.

---

### 5.2 Operational Next Steps for GTA: San Andreas Recompilation

Now that the agent architecture and skill suite are standardized, the project can resume advancing the authentic game execution frontier:

1. **Advance from Gate M8 (Engine Init / Main Loop) to Gate M9 (Interactive Gameplay)**:
   - Current status (`just frontier`): The runtime successfully executes `WinMainCRTStartup` (0x00824570), initializes Win32 and Tier 2 RenderWare rasters, and presents swapchain frames.
   - Target: Connect input polling to menu navigation and game state progression.

2. **Triage CRT Startup Indirect Calls (`$recomp-debug` + `$binary-analysis`)**:
   - `0x00824570` currently logs 10 unresolved CRT helper calls (`0x0082C471`, `0x008244F6`, `0x00827D25`, `0x0082BEDA`, `0x0082C0B9`, `0x0082DBC8`, `0x0082DB26`, `0x0082D8F3`, `0x00823B76`, `0x0082D88A`).
   - Classify targets using the decision matrix in [docs/implementation/recurring-recipes.md](implementation/recurring-recipes.md) and bridge or lift them cleanly.

3. **Activate Interactive Menu Loop (`$static-recomp` + `$recomp-validation`)**:
   - Verify coverage into unreached registered functions:
     - `CMenuManager_ProcessInternal` (0x00573cf0)
     - `CMenuManager_DrawBackground` (0x00576b70)
     - `CPad_UpdatePads` (0x0053f3c0) / `InitialisePads` (0x00746a10)
   - Enable user input navigation via the SDL gamepad and keyboard shims in `src/platform/`.

4. **Advance Graphics Interception to Tier 3 (`$native-renderer`)**:
   - Progress from Tier 2 2D Im2D rasters to Tier 3 3D world geometry pipelines (`RpWorld` / `RpAtomic` draw command interception).

## Current-state addendum (2026-09-17)

The active inventory is the seven directories currently present under
`.agents/skills/`: `binary-analysis`, `static-recomp`, `recomp-debug`,
`recomp-validation`, `native-renderer`, `platform-port`, and `recomp-project`.
Each contains Purpose, When to Use, Inputs, Workflow, Evidence Rules,
Validation, Failure Handling, Anti-Patterns, Knowledge Capture, and Completion
Criteria. A scan found no GTA, Midnight Club, Forza, RexGlue, or concrete game
addresses in those active skill bodies.

The older names described in the historical inventory above are deleted in the
working tree and are not counted as active skills. Because the worktree also
contains unrelated runtime/codegen/renderer changes, this audit does not claim
a zero-functional-diff reorganization. No runtime, generated code, renderer,
or project knowledge file was moved or deleted by this audit.
