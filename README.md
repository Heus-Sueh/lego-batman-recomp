# Configurable PC Static Recompilation

An experimental x86-32 static recompilation project powered by
[pcrecomp](https://github.com/sp00nznet/pcrecomp). The current target is
**LEGO Batman: The Videogame**, selected by [config/target.toml](config/target.toml).
Reusable infrastructure is being extracted for other games. Changing the
manifest alone does not port a new game.

## Configuration and ownership

The target manifest defines identity, output name, original executable name,
default game directory, active revision and window/simulation defaults.
CMake generates a typed C++ header from it. Binary-specific PE layout, entry
points, functions and globals belong in the selected directory under
`config/revisions/`.

Original application logic must execute through lifted instructions. Host
replacements belong at external boundaries such as Win32, graphics, input,
audio and filesystem access. Never hand-edit `generated/recomp/`.
Legacy GTA-specific hooks, addresses, asset readers and synthetic bridges
remain in the integration. They are migration debt, not evidence of correct
LEGO Batman behavior. See the [runtime audit](docs/runtime-architecture-audit.md).

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
at the paths in CMakeLists.txt. A Windows preset exists, but the game build still
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

## Validation and current observations

Current strict recompilation status: automatic return-only guest bridges are
disabled for LEGO Batman. There are 49 registered lifted functions; entry now
stops at the real missing CRT target 0x0074797F. Build succeeds, but the boot
integration tests no longer pass. See the [strict CRT investigation](docs/investigations/0016-lego-strict-crt-frontier.md).
The observations below describe the earlier bridge-assisted baseline only.

During the configuration migration on 2026-10-03, Linux generation, compilation
and bounded headless startup passed. The baseline frontier reported 48 registered
functions, 27 reached, zero unresolved indirect calls, a clean exit and zero
swapchain presentations. The Python suite passed 17/20 tests; three windowed
checks failed with SDL's `No available video device` in that session.

These observations do not prove correct rendering or interactive gameplay.
Use `just frontier` for current execution evidence. Historical GTA investigations,
old milestone percentages and synthetic probes are not current gate evidence.

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
The historical `legacy_pc` adapter remains selectable for migration comparisons.
See [target adapters](docs/architecture/target-adapters.md)
for ownership and remaining revision-specific migration. Original application
logic remains lifted code.
See [AGENTS.md](AGENTS.md), the [runtime architecture](docs/runtime-architecture.md)
and the [documentation guide](docs/README.md).

## Git and distribution

This checkout already has local Git history. No remote was configured at the
time of the initial documentation update; `origin` now points to
[Heus-Sueh/lego-batman-recomp](https://github.com/Heus-Sueh/lego-batman-recomp).
Build outputs, generated code and `game/` are ignored.
Users must supply their own legally obtained game files; this project does not
distribute the game.
