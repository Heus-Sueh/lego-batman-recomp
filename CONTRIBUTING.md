# Contributing

Thank you for helping build an authentic, reproducible static recompilation.
The project is experimental; consult the [README status](README.md#project-status)
and run `just frontier` before choosing execution-progress work. Read
[AGENTS.md](AGENTS.md) for repository rules. Write tracked code comments,
documentation, commit descriptions and technical evidence in English.

## Choose a contribution

- Without game files: improve reusable runtime libraries, synthetic regression
  tests, dependency boundaries, tooling or documentation.
- With a compatible, legally obtained executable: investigate the earliest
  unresolved call, verify function boundaries/ABI and extend revision metadata.
- For substantial new capabilities: discuss the scope in an issue and define
  observable acceptance criteria before implementing a subsystem.

Do not start with speculative renderer or gameplay implementations. Original
game logic must come from lifted instructions, not handwritten replacements.
Small, evidence-backed changes are easier to review than bulk function discovery.

## Set up a checkout

Fork the repository on GitHub, then clone your fork (replace YOUR_USERNAME):

```sh
git clone --recurse-submodules https://github.com/YOUR_USERNAME/lego-batman-recomp.git
cd lego-batman-recomp
git remote add upstream https://github.com/Heus-Sueh/lego-batman-recomp.git
git switch -c codex/describe-your-change
```

For an existing checkout, inspect `git status` first and preserve unrelated work.
Initialize missing submodules with `git submodule update --init --recursive`.
Do not update the pinned toolbox revision as an incidental part of another fix.

### Runtime-only development (no retail assets)

Install CMake 3.25+, Ninja, `just` and a C++23-capable compiler. From the repository
root, run:

```sh
just test-runtime
```

This configures `PCRECOMP_BUILD_GAME=OFF`, builds the reusable libraries and runs
their CTest suite. It does not require SDL, DXVK, generated game code or game files.
Tests live under `tests/runtime/`; register new tests in its `CMakeLists.txt`.

### Full game integration (currently validated on Linux x86_64)

In addition, install Python 3.11+, SDL3 development files and the DXVK-native
headers/library required by `src/adapters/common/bridge_dependencies.cmake`.
The expected library is `thirdparty/dxvk-native/lib/libdxvk_d3d9.so`; headers
are under `thirdparty/dxvk-native/include/`. Submodule initialization alone does
not provision that native library. Check those dependencies before configuring.
The Windows preset is not evidence of a working Windows game build.

```sh
python3 -m venv .venv
.venv/bin/python3 -m pip install pefile capstone
```

Supply your own game files under `game/`, as selected by `config/target.toml`.
Check `config/revisions/lego_batman/revision.toml` and measure the executable hash:

```sh
sha256sum game/LEGOBatman.exe
```

The input used for the documented frontier has SHA-256
`981383d8c842a919dfecf61caf808a27745c3af39f4fa6197e3da88f5e6c842f`.
Matching filenames, file size or PE headers alone do not establish identity.
If your hash differs, investigate the revision before applying these addresses;
do not bypass the checks or request/upload a retail executable in an issue.

```sh
just lift
just configure
just build
just frontier
```

The last command currently stops at an unresolved call and returns nonzero.
Record that baseline before making changes. For bounded headless execution:

```sh
.venv/bin/python3 scripts/tools/recomp_tool.py run --headless --frames 1
```

## Make a focused change

1. Record the command, revision/hash, caller, target, registers where relevant,
   expected result and actual result. Do not infer current progress from old logs.
2. Inspect original instructions with `just inspect <address>`. Verify boundaries,
   return instructions, argument cleanup and callers; discovery is a heuristic.
   In particular, thunk inspection can report zero size and print adjacent code.
3. Classify the failure as UNKNOWN until evidence identifies RECOMPILER, RUNTIME
   or GAME-SPECIFIC ownership. Fix the cause in the correct layer.
4. Make the smallest change with an observable success criterion. For substantial
   features, capture the contract in `specs/` and a bounded task in `tasks/`.
5. Regenerate, build and validate. Keep the next unresolved dependency visible;
   never add a success-returning stub merely to advance the counter.
6. Capture durable findings in `docs/`; use the
   [investigation template](docs/investigations/TEMPLATE.md) for frontier work.

### Where changes belong

| Change | Location |
|---|---|
| Target identity and default paths | `config/target.toml` |
| Binary-specific functions, ABI and globals | Selected `config/revisions/` directory |
| Generic instruction lifting/code generation | `thirdparty/pcrecomp/`, or project generator glue in `scripts/analysis/lifter/` |
| Reusable runtime, Win32 services, host primitives | `libpcrecomp/`, `win32compat/`, `pcmodern/` |
| Target integration and external boundary adapters | `src/adapters/`, `src/runtime/`, `src/win32/`, `src/platform/`, `src/renderer/` |
| Regression tests and investigation evidence | `tests/`, `docs/` |

Never hand-edit `generated/recomp/`. Fix metadata or the generator and run
`just lift`. For submodule changes, explain the upstream patch and how reviewers
can obtain it; do not submit an inaccessible local submodule commit.
Do not enable `RECOMP_LEGACY_GAME_HOOKS` for LEGO Batman. Preserve original
simulation pacing; a presentation-rate change must not uncap guest simulation.

## Validate and report limitations

Run the commands relevant to the change:

```sh
just test-runtime
just lift
just build
just test
just frontier
git diff --check
```

Runtime-only contributors need not run the game-dependent commands. Documentation
changes should verify links, commands and any status numbers they modify.
The Python suite currently has known failures; compare before and after rather
than treating every failure as new or every existing failure as harmless.
Do not disable assertions or replace expected behavior just to make tests pass.
Report unavailable game files, displays, GPUs or Windows toolchains separately.

For generic fixes, add a minimal regression test using synthetic data, with no
retail assets. For ABI work, check stack restoration and a meaningful register
or memory result. Test a negative control in an isolated path to show the check
can detect the defect. An optional GDB probe for the current initialization
leaves is documented in `tests/probes/lego_init_frontier.gdb` and requires game
files plus permission to debug a process.

Use VERIFIED, PARTIALLY_VERIFIED, INFERRED and UNKNOWN precisely. Follow the
[validation policy](.agents/skills/recomp-validation/SKILL.md): a successful build
does not prove initialization, rendering, gameplay or equivalence. Function-count
percentages are not completion estimates.

## Submit an issue or pull request

For a bug report, include the host OS/architecture, source commit and relevant
local changes, target manifest/revision/hash, reproduction command, expected and
actual behavior, and a short sanitized diagnostic excerpt. Do not attach full
memory dumps or proprietary data.

Before submitting a pull request:

- Explain the problem, ownership decision and scope; link the issue if one exists.
- List changed metadata addresses and their evidence, if applicable.
- List commands run and their results, including failures and skipped checks.
- State before/after frontier and highest validation level when changing execution.
- Include regression tests and update relevant documentation.
- Inspect `git diff` and `git diff --cached`; stage only the intended files.
- Exclude game binaries/assets, decrypted dumps, raw copyrighted bytes, generated
  code, build outputs, private traces and machine-specific absolute paths.

Commit the reviewed files, push your branch to your fork, and open a pull request
against this repository's default branch. Keep unrelated cleanup in separate
changes. Do not use a blanket `git add .` on a dirty checkout.

You do not need to solve an entire subsystem: a reproducible failure, a validated
function boundary, a small regression test or a documentation correction is useful.
