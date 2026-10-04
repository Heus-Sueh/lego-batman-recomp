# Build-selected target adapters

## Current status — 2026-10-04

Only `lego_batman` is accepted by CMake. The legacy adapter, RW/asset sources and
unused legacy generator bodies have been removed. The common source manifest
links the runtime/platform boundary plus the three extracted libraries described
in their READMEs. `RECOMP_LEGACY_GAME_HOOKS` is not enabled by any supported build.
Automatic purge-only fallbacks remain disabled for LEGO Batman.

See the root README for current frontier/test counts. All sections below are
**historical extraction records**, not descriptions of available adapters,
dependencies or current validation. `legacy_pc` is no longer selectable.

## Historical extraction — 2026-10-03

Current execution supersedes the earlier baseline below: LEGO Batman generation
now disables automatic purge-only guest bridges and exposes missing CRT code.
See [strict CRT frontier](../investigations/0016-lego-strict-crt-frontier.md).

The target manifest selects `adapter = "lego_batman"`. CMake resolves this through
an explicit allowlist and rejects unsupported adapters. Add a new entry and its
source/dependency manifest when a new integration is ready. `legacy_pc` remains
available as a historical migration baseline. Generic library-only
builds return before selecting or loading a game adapter.

`src/main.cpp` calls the `pcrecomp::adapter::run` interface. The selected adapter
owns startup orchestration, command-line handling, integration diagnostics,
guest entry dispatch and shutdown. Its `adapter.cmake` owns the legacy source
list, SDL/DXVK dependencies and graphics library search paths.

The first extraction intentionally retains existing code in
`src/adapters/legacy_pc/application.cpp`. The name describes its provenance:
it includes GTA-era memory injections and probes alongside LEGO Batman entry
dispatch. These are not newly validated LEGO Batman behaviors. Shared legacy
source files and generator bridges still contain title-specific assumptions;
this extraction establishes an ownership boundary, not complete portability.
The independent `libpcrecomp`, `win32compat` and `pcmodern` libraries are unchanged.

Future migration must verify each inherited hook against the selected binary,
move proven addresses to revision metadata and eliminate synthetic game logic.
Do not merely rename this integration to a native TT Games adapter or activate
GTA asset readers for another title. See `specs/target-adapters.md`.

## Validation — 2026-10-03

The list below records the initial legacy extraction. See the LEGO Batman
follow-up section for the current adapter results.

- VERIFIED: Linux configure/build and entry execution (Level 3).
- VERIFIED: bounded frontier unchanged: 48 registered, 27 reached, 30 dispatches,
  zero unresolved calls; current VA 0x006e3980, last successful VA 0x005290a0.
- VERIFIED: all four reusable runtime tests passed.
- VERIFIED: integration suite retains 17 passing tests. Three windowed tests
  remain BLOCKED_ENVIRONMENT (`No available video device`).
- VERIFIED: isolated manifest with `unknown_probe` fails configuration with
  `Unsupported target adapter`, serving as a negative selection control.
- UNKNOWN: visual correctness, behavioral equivalence and Windows game build.
  No milestone advancement is claimed.

## LEGO Batman bootstrap adapter — 2026-10-03

`src/adapters/lego_batman/application.cpp` now owns this title's bootstrap.
Entry VA, image base, expected file size and host stack policy come from the
selected revision via a configured header. The PE supplied at runtime must match
size, i386/PE32 signatures, image base and entry VA. These checks are structural,
not a full-file cryptographic identity check. Same-size code modifications remain
undetected. The local reference inspected with pefile had entry 0x0073d6a3,
base 0x00400000 and size 7557120 (VERIFIED header evidence).

The adapter honors an explicit game directory without falling back to another
installation. It rejects missing images, unsupported options and malformed bounds.
It initializes memory and Win32 boundaries and dispatches the revision's lifted
entry, with cleanup on failures. It does not initialize RW, load GTA assets or
inject guest menu/window flags. `--frames` bounds message-pump polls; it does
not count rendered frames. `--fps` and legacy graphical probes are unsupported.

SDL and Win32 direct guest writes are compiled only under
`RECOMP_LEGACY_GAME_HOOKS`, enabled exclusively for the legacy adapter. Real
guest input delivery and Win32 handle/callback fidelity remain runtime work.
The LEGO adapter sets the host window title from the target manifest.

The shared `common/bridge_dependencies.cmake` still links RW/asset code needed
by emitted legacy bridge bodies. Generated purge-only fallback bridges also
remain; absence of unresolved calls does not establish authentic execution of
all original callees. Those existing generator limitations are not resolved or
promoted by introducing this adapter. No generated artifact was hand-edited.

VERIFIED: both adapters build on Linux; LEGO bounded entry retains 48 registered,
27 reached, 30 dispatches, zero unresolved calls and the previous frontier.
VERIFIED: four reusable runtime tests pass. Integration tests with access to the
host display pass 19 checks, including five adapter contract tests; six explicitly
legacy-only diagnostics are skipped for the LEGO profile. The contract tests
include synthetic invalid-PE and missing-directory negative controls.
The two applicable windowed checks fail without display access in the sandbox
and pass outside it. They verify host initialization, not expected game rendering.
Highest execution claim remains Level 3 (Entry); visual/gameplay equivalence and
Windows builds remain UNKNOWN. No Windows cross-compiler was available.
