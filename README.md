# Configurable PC Static Recompilation

An experimental x86-32 static recompilation project powered by
[pcrecomp](https://github.com/sp00nznet/pcrecomp). The current target is
**LEGO Batman: The Videogame**, selected by [config/target.toml](config/target.toml).
Reusable infrastructure is being extracted for other games. Changing the
manifest alone does not port a new game.

## Project status

**Experimental and not playable yet.** Snapshot checked on **2026-10-04**
against the configured LEGO Batman revision. Natural execution reaches the
original entry point and part of application initialization, then stops at an
unresolved call. Complete initialization, a correct first frame and interactive
gameplay have not been demonstrated.

| Measurement | Current result | Meaning |
|---|---:|---|
| Automatically discovered function candidates | 23,698 | Analysis inventory, not a verified total of required game functions |
| Selected, lifted and registered functions | 300 | Generated from the original executable and included in the Linux build |
| Inventory selection | 1.27% | 300 / 23,698 candidates; **not game completion** |
| Candidates not yet selected | 23,398 (98.73%) | Still require triage; not necessarily all needed or correctly delimited |
| Registered functions reached in bounded execution | 228 / 300 (76.0%) | Coverage of the selected subset, not the whole game |
| Registered functions not reached in that run | 72 | Already selected; not an additional count of functions left to recompile |
| Function dispatches | 2,912 | Calls observed before the current blocker |

Counts come from `config/revisions/lego_batman/functions.json`, the selected
`functions.toml`, and `just frontier`. Automated discovery can include incorrect
boundaries, duplicate/overlapping regions or code that is never needed. It may
also miss functions. **The exact amount of work remaining and overall completion
percentage are unknown.** Function sizes and implementation difficulty vary.

The immediate blocker is `0x00401587`, called by `0x004B01D0`; the target is a
jump to `0x00408900`, whose body and ABI need investigation. The last successful
function is `0x005C8110`. Automatic return-only guest bridges are disabled for
LEGO Batman; missing calls remain visible failures.
See the [latest initialization investigation](docs/investigations/2026-10-03-lego-init-leaves.md).

Remaining work includes expanding authentic code coverage along the execution
frontier, validating the Win32/graphics/input/audio boundaries as they are reached,
completing initialization, demonstrating a correct first frame and gameplay, and
comparing behavior with the original. Windows game builds remain unvalidated.
There is no completion date estimate.

Want to help? Start with [CONTRIBUTING.md](CONTRIBUTING.md), including a
runtime-only workflow that does not require proprietary game files.

## Configuration and ownership

The target manifest defines identity, output name, original executable name,
default game directory, active revision and window/simulation defaults.
CMake generates a typed C++ header from it. Binary-specific PE layout, entry
points, functions and globals belong in the selected directory under
`config/revisions/`.

Original application logic must execute through lifted instructions. Host
replacements belong at external boundaries such as Win32, graphics, input,
audio and filesystem access. Never hand-edit `generated/recomp/`.
Historical GTA documentation and the legacy comparison profile are not evidence
of correct LEGO Batman behavior. See the [runtime audit](docs/runtime-architecture-audit.md)
for migration context, and inspect current source before relying on its findings.

The default expects user-supplied `game/LEGOBatman.exe` and game data.
Verify binary identity against the selected revision before using its addresses.
Do not commit proprietary executables, assets, decrypted dumps or local paths.

After changing the default manifest, run `just lift`, `just configure` and
`just build`. Alternate manifests currently need explicit arguments to CMake
(`-DRECOMP_TARGET_CONFIG=...`) and the lifter (`--target-config ...`). The
 developer CLI and integration tests still read `config/target.toml`.
Selection is not yet unified across tools; generated sources share one output
directory, so simultaneous builds for different games are not supported.

## Build and run

The active build path is Linux x86_64 with a C++23 compiler, CMake 3.25+, Ninja,
Python 3.11+ (`tomllib`), `pefile`, `capstone`, `just` and SDL3 development
files. Game integration also requires DXVK-native headers and its D3D9 library
at the paths in `src/adapters/common/bridge_dependencies.cmake`. A Windows preset exists, but the game build still
contains Linux-specific library paths and has not been validated on Windows.

```sh
git submodule update --init --recursive
python3 -m venv .venv
.venv/bin/python3 -m pip install pefile capstone
just lift
just configure
just build
just frontier
```

`just run` launches the configured executable. For bounded headless execution:

```sh
.venv/bin/python3 scripts/tools/recomp_tool.py run --headless --frames 1
```

Consult `--help` for runtime options. Preserve the original simulation timing
when changing presentation pacing. The configured 30 FPS default does not
establish the timing model of every game.

## Validation

- Linux generation passed in the latest code-validation session (2026-10-03);
  the build was rechecked on 2026-10-04 with no work pending. Existing
  unused-function warnings remain from compilation.
- `just test-runtime`: **6/6 passed**, rechecked 2026-10-04, including PE-data
  preservation, allocator bounds and shutdown/reinitialization.
- `just test`: **10/22 passed, 12 failed**, rechecked 2026-10-04. Failures include
  stale frontier expectations, tests requiring execution beyond the unresolved
  call, and unavailable sandbox video. The suite is not green.
- Targeted debugger probes verify the two latest lifted functions' stack cleanup
  and memory postconditions. This is not proof of complete initialization.

Highest demonstrated overall validation level: **Level 3 (natural entry)**,
with targeted initialization checks. Rendering, gameplay and comparative
equivalence remain unproven. Use `just frontier` for live execution evidence;
its nonzero exit currently reports the known blocker, not a successful boot.

| Command | Purpose |
|---|---|
| `just test-runtime` | Test reusable libraries without SDL, DXVK or retail files |
| `just test` | Integration tests; several require game files or a display |
| `just frontier` | Bounded execution and coverage |
| `just inspect <address>` | Inspect original instructions at a guest address |
| `just trace [frames]` | Trace bounded execution |
| `just cycle` | Lift, build, test and trace |

Reusable libraries live in `libpcrecomp/`, `win32compat/` and `pcmodern/`.
The manifest now selects the `lego_batman` integration adapter. Its bootstrap
uses revision-derived PE entry metadata, checks PE headers and file size, and
does not initialize GTA/RenderWare assets or inject GTA input/window globals.
Legacy raster/menu/splash probes and `--fps` are unsupported in this adapter.
The historical `legacy_pc` adapter has been removed and is no longer selectable.
See [target adapters](docs/architecture/target-adapters.md)
for ownership and remaining revision-specific migration. Original application
logic remains lifted code.
All three extracted libraries are linked into the game: see
[libpcrecomp](libpcrecomp/README.md), [pcmodern](pcmodern/README.md) and
[win32compat](win32compat/README.md) for consumers, tests and migration limits.
See [AGENTS.md](AGENTS.md), the [runtime architecture](docs/runtime-architecture.md)
and the [documentation guide](docs/README.md).

## Git and distribution

Project repository: [Heus-Sueh/lego-batman-recomp](https://github.com/Heus-Sueh/lego-batman-recomp).
Build outputs, generated code and `game/` are ignored.
Users must supply their own legally obtained game files; this project does not
distribute the game.
