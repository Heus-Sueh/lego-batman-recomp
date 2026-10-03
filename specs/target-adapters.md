# Target adapter extraction

Scope: move the existing application integration behind a build-selected adapter.
The launcher must contain no guest addresses, RenderWare calls or title-specific
diagnostics. The adapter owns its source list and external graphics dependencies.
The default manifest explicitly selects the legacy integration; unknown adapters
must fail configuration. Generic library-only builds must remain independent.

Acceptance: Linux build, unchanged bounded frontier (48 registered, 27 reached,
30 dispatches, zero unresolved calls), existing headless regression tests and a
negative configure check for an unknown adapter. This is ownership extraction,
not validation of inherited GTA addresses against LEGO Batman. No generated
code changes or original game logic replacements are required.

## LEGO Batman adapter

Select `lego_batman` explicitly. Startup must load the selected revision's PE,
validate its size, i386 machine, image base and entry point before dispatch, and
use revision-derived entry metadata. Missing or mismatched files must fail.
The bootstrap must not write GTA globals or initialize RenderWare assets.
SDL/Win32 GTA memory injections must compile only for `legacy_pc`.
Legacy menu/splash/raster probes must be rejected explicitly by this adapter.
Retained generated bridges may still link legacy graphics dependencies; removing
all unused generator bodies is a separate migration with binary evidence.
