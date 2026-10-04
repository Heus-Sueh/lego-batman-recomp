# Build-Selected Target Adapters

The manifest selects `adapter = "lego_batman"`, currently the only supported
adapter. CMake rejects other names. The former comparison adapter was removed.
Library-only builds do not load a game adapter.

`src/main.cpp` calls `pcrecomp::adapter::run`. The adapter owns CLI validation,
startup, diagnostics, guest entry dispatch and cleanup. Its CMake manifest
selects project sources and platform dependencies.

The current bootstrap uses revision-derived entry/base/size/stack policy.
Structural PE checks do not establish cryptographic identity; verify the input
hash before applying revision metadata. Explicit game directories do not
silently fall back. Missing images and invalid options are rejected.

`--frames` bounds message-pump polls, not rendered frames. `--fps` and former
menu/raster/splash diagnostics are unsupported. No supported build enables
`RECOMP_LEGACY_GAME_HOOKS`.

Adding a target requires binary identity, metadata, consistent selection and
isolated output across tools. Reuse libraries, not another title's globals.
Keep application behavior lifted and boundary code separate.

See [runtime architecture](../runtime-architecture.md),
[contributing](../../CONTRIBUTING.md) and
[current validation](../../README.md#validation).
