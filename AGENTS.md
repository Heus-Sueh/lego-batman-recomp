# AGENTS.md — Configurable PC Static Recompilation

## 1. Project Identity & Objective

This repository develops a native static recompilation of a configured x86-32 PC game using pcrecomp. The current target is LEGO Batman: The Videogame. Read `config/target.toml` for identity and revision selection; historical GTA documentation is not current target evidence.

```text
configured original executable
    ↓
PE analysis (pe_analyze.py)
    ↓
x86-32 disassembly (disasm32.py)
    ↓
function / data discovery (config/revisions/)
    ↓
pcrecomp lifting (lift32.py / run_lift.py)
    ↓
generated C/C++ (generated/recomp/)
    ↓
project runtime + platform layer (src/)
    ↓
native executable (Windows x86_64 / Linux x86_64)
```

The original executable is the authoritative behavioral reference for original game logic.

The project ships no copyrighted game assets or retail executables. Users must supply their own compatible PC binary.

All tracked project content must be written in English.

---

## 2. Core Project Invariants

### 2.1 Recompilation, Not Recreation

The goal is to execute authentic x86-32 machine instructions lifted from the original game binary.

**Do not rewrite, recreate, approximate, or mock original game logic in hand-written C++ under `src/game/` or elsewhere.**

Original game behavior belongs to lifted code.

See:

[docs/decisions/0001-recompilation-vs-recreation.md](docs/decisions/0001-recompilation-vs-recreation.md)

### 2.2 Boundary Replacement Policy

Recompile original application logic first.

Replace only external host boundaries when necessary, including:

* Win32;
* DirectInput;
* DirectSound;
* DirectX / RenderWare presentation boundaries;
* host filesystem integration;
* threading;
* platform-specific services.

Do not replace original game logic merely because implementing the corresponding compatibility boundary is difficult.

See:

[docs/architecture/win32-compatibility.md](docs/architecture/win32-compatibility.md)

### 2.3 Clean Binary Requirement

Encrypted or protected executables cannot automatically be treated as ordinary linearly disassemblable images on disk.

Disassembly and lifting require a compatible clean executable or decrypted memory image matching the selected revision. Verify binary identity before applying addresses or patches. The linked GTA DRM notes are historical title-specific guidance.

See:

[docs/gotchas/drm-crack-trampolines.md](docs/gotchas/drm-crack-trampolines.md)

### 2.4 Generated Code Ownership

Files under:

```text
generated/recomp/
```

are generated artifacts.

**Never edit generated C/C++ files manually.**

Fixes belong, in order of preference, in:

1. revision metadata under `config/revisions/`;
2. reusable upstream lifter/codegen logic under `thirdparty/pcrecomp/`;
3. deterministic preimage-checked patches in `patches.toml`, when necessary.

A generated-code problem should be fixed at its source so regeneration preserves the fix.

### 2.5 Simulation and Presentation Pacing

The target manifest currently defaults to 30 simulation ticks per second. Establish each game's original timing model from binary evidence before changing it.

Do not uncap guest simulation ticks merely to increase presentation frame rate.

Host presentation may run independently using monotonic pacing where supported.

See:

[docs/decisions/0002-decoupled-presentation-pacing.md](docs/decisions/0002-decoupled-presentation-pacing.md)

---

## 3. Failure Ownership

Every unresolved failure begins as:

```text
UNKNOWN
```

After investigation, classify it as one of:

```text
RECOMPILER
RUNTIME
GAME-SPECIFIC
```

Do not classify failures based only on where the crash becomes visible.

### Golden Rule

Ask:

> Is this failure caused by a general property of x86/Win32 execution, recompilation, ABI translation, or host portability rather than behavior unique to the selected game?

If yes, prefer fixing the reusable recompiler or runtime layer.

If uncertain, keep the issue classified as `UNKNOWN` and gather evidence before assigning ownership.

Never move Title-specific behavior into the generic runtime merely to make the current title work.

Likewise, never introduce game-specific workarounds for defects that belong to the generic recompiler/runtime.

---

## 4. Repository Layout

```text
.
├── AGENTS.md
│   # Repository-specific instructions for agents and developers
│
├── CMakeLists.txt
│   # Primary build configuration
│
├── CMakePresets.json
│   # Build presets
│
├── justfile
│   # Developer task runner
│
├── config/
│   └── revisions/
│       └── <active_revision>/
│           # Active executable revision metadata
│
├── docs/
│   ├── architecture/
│   │   # Runtime and title-specific subsystem architecture
│   │
│   ├── decisions/
│   │   # Architectural Decision Records
│   │
│   ├── gotchas/
│   │   # Reverse-engineering and runtime traps
│   │
│   ├── implementation/
│   │   # Project-specific implementation guidance
│   │
│   ├── investigations/
│   │   # Historical execution-frontier investigations
│   │
│   └── references/
│       # External projects and architecture references
│
├── generated/
│   └── recomp/
│       # Generated lifted C/C++ — NEVER hand-edit
│
├── scripts/
│   ├── analysis/
│   │   # Analysis and lifting automation
│   │
│   └── tools/
│       # Development CLI helpers
│
├── src/
│   ├── platform/
│   │   # Host windowing, input and audio
│   │
│   ├── renderer/
│   │   # Native presentation and graphics integration
│   │
│   ├── runtime/
│   │   # Guest memory, register context and dispatch
│   │
│   └── win32/
│       # Win32 / DirectX compatibility boundaries
│
├── tests/
│   # Host-side automated tests
│
└── thirdparty/
    └── pcrecomp/
        # Pinned upstream recompiler toolbox
```

---

## 5. Starting Work

Read `config/target.toml` before editing target-dependent code. Identity and
default paths belong there; binary addresses, patches and ABI metadata belong
to the selected revision. Keep external boundary adapters separate from generic
libraries (`libpcrecomp/`, `win32compat/`, `pcmodern/`). Adapter extraction
must not introduce handwritten replacements for original game logic.

Configuration selection is currently partial: CMake and the lifter accept
alternate manifests, while the developer CLI and integration tests read the
default manifest. Generated sources share one output directory. Do not claim
multi-game support until selection, revision validation and output isolation
have been verified together.

The default integration adapter is `lego_batman`; `legacy_pc` is a historical
comparison profile. See `docs/architecture/target-adapters.md`. Never enable
`RECOMP_LEGACY_GAME_HOOKS` for LEGO Batman. Legacy menu/raster/splash tests are
not capabilities of this title. PE header checks do not establish full binary
identity, and existing generated purge bridges do not prove original behavior.

Preserve existing Git history and unrelated staged/unstaged changes. Never use
a blanket initial commit on an existing dirty checkout or include proprietary
files. Historical investigations remain historical; label their target and
revision when citing them.


Before modifying the project:

1. Read this file.
2. Determine whether the task affects execution progress.
3. If it does, run:

```sh
just frontier
```

4. Identify the relevant subsystem.
5. Load the corresponding skill.
6. Read only the documentation relevant to the current problem.
7. Inspect the existing implementation before changing it.
8. Establish observable success criteria.
9. Make the smallest useful verifiable change.
10. Validate before moving to the next step.

Do not read the entire knowledge base by default.

Do not implement later-stage systems merely because they will eventually be needed.

---

## 6. Current Project State

The canonical current execution state is obtained with:

```sh
just frontier
```

Use it before work involving:

* boot progression;
* execution frontier;
* unresolved indirect calls;
* initialization;
* runtime blockers;
* milestone advancement.

Do not infer current project progress from historical investigation documents.

Files under:

```text
docs/investigations/
```

describe previous investigations and may no longer represent the current execution state.

Think of the distinction as:

```text
docs/investigations/ → historical knowledge
just frontier        → current execution state
```

When they disagree, investigate the current executable/runtime behavior before drawing conclusions.

---

## 7. Execution Frontier Policy

When advancing boot or runtime progress, prioritize the **earliest demonstrated execution blocker**.

Preferred workflow:

```text
current frontier
    ↓
identify blocker
    ↓
collect evidence
    ↓
classify failure
    ↓
select relevant skill
    ↓
resolve root cause
    ↓
validate
    ↓
advance frontier
```

Do not work ahead of the execution frontier unless:

* the task explicitly requires it;
* a prerequisite must be implemented first;
* independent work has clear validation;
* or evidence shows that later work directly blocks the current frontier.

Avoid speculative implementation of systems that execution has not reached.

---

## 8. Key Commands

| Command               | Purpose                                                                  |
| --------------------- | ------------------------------------------------------------------------ |
| `just build`          | Build runtime and lifted code using the configured build system.         |
| `just run`            | Launch the recompiled executable.                                        |
| `just frontier`       | Show the current execution frontier and boot-gate progress.              |
| `just inspect <addr>` | Inspect/disassemble a guest address or function boundary.                |
| `just cycle`          | Run the primary lift → build → test → frontier development cycle.        |
| `just trace [N]`      | Run a trace for N frames/steps and expose unresolved execution behavior. |
| `just test`           | Run tests that do not require proprietary retail assets.                 |

Prefer these canonical commands over manually reproducing their internal steps unless debugging the commands themselves.

---

## 9. Skill Routing

Project skills live under:

```text
.agents/skills/
```

Use specialized skills instead of duplicating their methodology inside this file.

| Responsibility                                                               | Skill                                                             |
| ---------------------------------------------------------------------------- | ----------------------------------------------------------------- |
| Project coordination, milestone gates and recompilation lifecycle            | [`$recomp-project`](.agents/skills/recomp-project/SKILL.md)       |
| PE analysis, x86 disassembly, calling conventions and ABI investigation      | [`$binary-analysis`](.agents/skills/binary-analysis/SKILL.md)     |
| Instruction lifting, CFG recovery, jump tables and generated-code boundaries | [`$static-recomp`](.agents/skills/static-recomp/SKILL.md)         |
| Execution-frontier debugging, crashes, stack imbalance and unresolved calls  | [`$recomp-debug`](.agents/skills/recomp-debug/SKILL.md)           |
| Evidence classification, validation ladder and regression validation         | [`$recomp-validation`](.agents/skills/recomp-validation/SKILL.md) |
| Renderer migration, presentation decoupling and graphics integration         | [`$native-renderer`](.agents/skills/native-renderer/SKILL.md)     |
| SDL host integration, windowing, input, audio and compatibility boundaries   | [`$platform-port`](.agents/skills/platform-port/SKILL.md)         |

Use `$recomp-project` for high-level coordination.

Use specialized skills for implementation and investigation.

Do not duplicate specialized procedures in `AGENTS.md`.

---

## 10. Documentation Map

Consult targeted documentation before modifying the corresponding subsystem.

### Architecture

RenderWare:

[docs/architecture/renderware-subsystems.md](docs/architecture/renderware-subsystems.md)

Win32 compatibility:

[docs/architecture/win32-compatibility.md](docs/architecture/win32-compatibility.md)

### Implementation

Recurring resolution patterns:

[docs/implementation/recurring-recipes.md](docs/implementation/recurring-recipes.md)

### Gotchas

Stack and ABI:

[docs/gotchas/stack-balance-and-abi.md](docs/gotchas/stack-balance-and-abi.md)

DRM and crack trampolines:

[docs/gotchas/drm-crack-trampolines.md](docs/gotchas/drm-crack-trampolines.md)

Guest simulation pacing:

[docs/gotchas/guest-simulation-pacing.md](docs/gotchas/guest-simulation-pacing.md)

### External References

pcrecomp upstream:

[docs/references/pcrecomp-upstream.md](docs/references/pcrecomp-upstream.md)

### Historical Investigations

[docs/investigations/](docs/investigations/)

Investigation journals are historical evidence, not authoritative statements of current progress.

---

## 11. Validation Requirements

This project uses the validation methodology defined by:

[`$recomp-validation`](.agents/skills/recomp-validation/SKILL.md)

Do not duplicate the validation ladder here.

Never claim a higher validation level from lower-level evidence.

For example:

```text
successful generation
    ≠ successful build

successful build
    ≠ successful entry

successful entry
    ≠ successful initialization

successful initialization
    ≠ correct rendering

correct rendering
    ≠ correct gameplay

correct gameplay
    ≠ behavioral equivalence
```

Every address, symbol, calling convention, ABI assumption, or original-binary behavioral claim must follow the evidence policy defined by `$recomp-validation`.

Important claims must remain explicitly distinguishable as:

```text
VERIFIED
PARTIALLY_VERIFIED
INFERRED
UNKNOWN
```

---

## 12. Milestone Gates

The canonical milestone gates (`M0`–`M9`) are defined and managed by:

[`$recomp-project`](.agents/skills/recomp-project/SKILL.md)

Do not redefine the milestone model in this file.

When working on execution progression:

1. identify the current milestone;
2. identify its active gate;
3. identify the earliest blocker;
4. resolve that blocker;
5. validate the gate;
6. advance only when the gate's completion criteria are satisfied.

Do not mark a milestone complete based solely on compilation or partial execution.

---

## 13. Change Ownership

When a problem is found in generated code, determine where the fix actually belongs.

Use this ownership order:

```text
binary/revision metadata issue
        ↓
config/revisions/

generic lifting/codegen issue
        ↓
thirdparty/pcrecomp/

generic execution/runtime issue
        ↓
src/runtime/ or compatibility layer

host platform boundary
        ↓
src/platform/, src/win32/, src/renderer/

Title-specific metadata/knowledge
        ↓
revision metadata or docs/

generated artifact
        ↓
NEVER hand-edit
```

Do not patch the closest visible symptom merely because it is easier.

Fix the lowest correct ownership layer.

---

## 14. Knowledge Capture

Do not force future agents to rediscover durable project knowledge.

When an investigation produces reusable Title-specific knowledge, update the appropriate location under `docs/`.

Examples:

```text
architecture discovery
    → docs/architecture/

architectural decision
    → docs/decisions/

reverse-engineering trap
    → docs/gotchas/

repeatable project-specific technique
    → docs/implementation/

temporary or evolving investigation
    → docs/investigations/

external implementation analysis
    → docs/references/
```

Generic reusable methodology belongs in a skill, not Title-specific documentation.

Do not duplicate the same knowledge across multiple documents when one canonical source can be referenced.

---

## 15. Definition of Done

Before declaring a change complete:

1. Re-read the requested task.
2. State which milestone gate or subsystem the change affects.
3. State the highest validation level actually demonstrated.
4. Build the affected configuration.
5. Run relevant tests.
6. Perform runtime validation when applicable.
7. Provide concise observable evidence.
8. Check for unresolved regressions.
9. Remove temporary debugging changes unless intentionally retained.
10. Update durable documentation when new knowledge was discovered.

For platform-level changes, validate both Linux and Windows build paths when feasible and relevant.

Do not claim completion while required behavior remains `UNKNOWN`.

---

## 16. Repository Hygiene

Do not commit:

* retail executables;
* proprietary game assets;
* decrypted binary dumps;
* raw copyrighted binary data;
* machine-specific absolute paths;
* temporary traces containing proprietary data;
* build artifacts unless explicitly tracked by the project.

Logs and documentation must avoid:

* proprietary asset contents;
* unnecessary raw binary dumps;
* developer-specific absolute paths;
* sensitive local environment information.

Keep generated artifacts clearly separated from maintained source code.

---

## 17. Guiding Principle

When uncertain, prefer:

```text
inspect
→ measure
→ classify
→ fix the correct layer
→ validate
→ document durable knowledge
→ advance the execution frontier
```

over:

```text
guess
→ patch symptom
→ add workaround
→ assume success
```

The objective is not merely to make the current executable progress further.

The objective is to produce a reproducible, maintainable static recompilation whose runtime behavior remains grounded in the original x86-32 program.
