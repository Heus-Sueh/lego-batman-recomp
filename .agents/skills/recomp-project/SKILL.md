---
name: recomp-project
description: "Orchestrate static recompilation projects across their lifecycle from PE binary analysis to interactive gameplay. Determines the active milestone gate, sequences specialized skills, and enforces recompilation invariants without duplicating low-level procedures."
---

# recomp-project: Static Recompilation Orchestration

## 1. Purpose
Act as the high-level coordinator for the static recompilation project. Determine the active project phase, enforce non-negotiable recompilation invariants, and route tasks to specialized skills without duplicating their technical procedures.

## 2. When to Use
- When initiating or planning work across the recompilation pipeline.
- When determining the project's current milestone gate or frontier.
- When deciding which specialized skill to activate for a given problem.
- When sequencing multi-step tasks across multiple subsystems.

**Specialized Skill Routing**:
- To analyze PE headers, disassemble x86 instructions, or discover function ABIs: use `$binary-analysis`.
- To lift instructions, reconstruct CFGs, recover jump tables, or generate C++: use `$static-recomp`.
- To triage crashes, debug the execution frontier, or diagnose unresolved ICALLs: use `$recomp-debug`.
- To verify behavior against the 7-level validation ladder or enforce evidence levels: use `$recomp-validation`.
- To handle graphics presentation, swapchain pacing, or renderer migration tiers: use `$native-renderer`.
- To implement host windowing, audio, input, or Win32 compatibility shims: use `$platform-port`.

## 3. Inputs
- Executable binary identity (SHA-256 hash, file size, PE timestamp).
- Current milestone gate status (see Section 4).
- Project revision configuration directory (`config/revisions/<revision>/`).
- Runtime execution logs and coverage traces.

## 4. Workflow

### Conceptual Project Lifecycle
```text
binary analysis
       ↓
 loader / runtime
       ↓
    imports
       ↓
   entrypoint
       ↓
 initialisation
       ↓
    renderer
       ↓
  first frame
       ↓
    gameplay
```

### Milestone Gates Definition
Progress is measured through verifiable, evidence-backed gates:

| Gate | Name | Verifiable Condition | Primary Skill |
|---|---|---|---|
| **M0** | **Binary Understood** | Executable SHA-256 verified, sections parsed, entry point and image base recorded. | `$binary-analysis` |
| **M1** | **Image Mapped** | Guest memory initialized, PE sections (`.text`, `.rdata`, `.data`, `.bss`) mapped into address space. | `$platform-port` |
| **M2** | **Relocations Working** | Base relocations applied (if ASLR/rebased) or fixed base address verified. | `$platform-port` |
| **M3** | **Imports Resolved** | Win32 / runtime imports bound to compatibility shims with matching stack cleanup. | `$platform-port` |
| **M4** | **Entrypoint Reached** | Native executable executes lifted PE entry point (`WinMainCRTStartup` / `mainCRTStartup`). | `$recomp-debug` |
| **M5** | **Runtime Initialized** | CRT startup progresses through initializers, heap setup, and reaches application entry (`WinMain`). | `$recomp-debug` |
| **M6** | **Game Init Reached** | Core application initialization begins (`CGame::Initialise` or title equivalent). | `$recomp-debug` |
| **M7** | **Renderer Initialized** | Graphics device and engine presentation structures initialized. | `$native-renderer` |
| **M8** | **First Frame** | First visual frame or splash screen presented to host swapchain. | `$native-renderer` |
| **M9** | **Interactive Gameplay** | The natural main loop runs repeatedly; a real host input traverses the production boundary and causes an observable guest gameplay transition; world state remains stable over a bounded run. | `$recomp-validation` |

### Orchestration Procedure
1. Identify the current active milestone gate (`M0`–`M9`) and the active execution frontier. Never jump ahead to late milestones (e.g., world loading) if earlier gates (e.g., game initialization) are incomplete.
2. Identify the **first proven divergence** or blocker blocking gate progression.
3. Determine whether a **Spec** is required:
   - *Substantial capability / subsystem*: Formulate or verify a verifiable contract in `specs/` (`SPEC-NNN`) defining falsifiable requirements, acceptance criteria, and non-goals before implementation.
   - *Isolated bug fix*: Proceed directly with task + minimal reproduction + regression test (`Spec: N/A`).
4. Derive concrete, bounded tasks under `tasks/` referencing the stable Spec ID (`Spec: SPEC-NNN`).
5. Route implementation to the single specialized skill corresponding to the blocker.
6. If runtime evidence contradicts spec assumptions, halt, revise the spec and tasks, and resume from the corrected contract.
7. Once the specialized task completes, re-evaluate the milestone gate with reproducible execution evidence (Level 3+ validation).
8. Decompose cross-boundary behavior into observable links. For input, use:
   `host device -> SDL event -> Win32 queue -> DispatchMessage -> guest callback -> guest state -> user-visible response`.
   Verify each link and identify bypassed links; a direct callback probe cannot
   close the host-device gate.
9. Treat forced flags, direct guest-memory injection, and diagnostic entry points
   as probes only. They may isolate a later stage but cannot prove that the natural
   earlier transition works.
10. Use the canonical M0–M9 model in this skill for current status. Older M0–M14
   labels may appear in historical investigations, README tables, logs, or version
   banners; treat them as legacy labels and identify the gate by name and evidence
   instead of silently translating or promoting it.

## 5. Evidence Rules
- A milestone gate is ONLY considered reached when backed by observable execution traces (Level 3+ validation).
- Code compiling and linking successfully only proves Level 2 (Build); it does not prove gate passage.
- Maintain status tags: `VERIFIED`, `PARTIALLY_VERIFIED`, `INFERRED`, or `UNKNOWN`.
- `just frontier` is authoritative for the current execution VA/failure, but it
  does not by itself prove visual correctness, physical input, audio, or gameplay.
- Report validation blocked by missing display/GPU/audio/toolchain separately from
  implementation failures; do not promote the gate while required evidence is absent.

## 6. Validation
- Run project test runner or headless execution (`just cycle` or equivalent).
- Verify that runtime reaches the expected guest virtual address (VA) and logs expected initialization markers.

## 7. Failure Handling
- If execution halts before the targeted milestone, classify the failure using the 3-Failure rule (`RECOMPILER`, `RUNTIME`, or `GAME-SPECIFIC`) via `$recomp-debug`.
- Never attempt to bypass a missing gate by fabricating synthetic mock state in host C++.

## 8. Anti-Patterns
- **The Speculative Leap**: Attempting to implement gameplay or renderer features before runtime reaches engine initialization.
- **Source Recreation**: Re-implementing game logic in hand-written C++ rather than statically recompiling original code.
- **Compilable != Functional**: Marking a gate complete simply because code compiles and links without running it.
- **Silent Stubs**: Bypassing failing subsystems with empty dummy functions that claim success.
- **Probe Promotion**: Treating a forced transition or direct callback invocation
  as proof that the natural end-to-end path works.

## 9. Knowledge Capture
At the conclusion of an orchestration phase:
- Record advanced milestones in project documentation (`docs/`).
- Document rejected alternatives and reasons in `docs/decisions/`.
- Update revision manifests in `config/revisions/` if new function ranges or imports were established.

## 10. Completion Criteria
- Current milestone gate status is explicitly updated with verifiable trace evidence.
- Required evidence links for the gate are listed as verified, partial, or blocked.
- The next execution frontier is defined with a concrete target guest address.
- All specialized tasks are routed to their respective skills.
