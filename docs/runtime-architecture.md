# Reusable PC Runtime Architecture

The three libraries are linked into the current game executable. Migration is
partial: they do not replace all generated ABI state or platform integration.

| Target | Responsibility | Dependencies |
|---|---|---|
| [pcrecomp::bridge](../libpcrecomp/README.md) | Context, checked memory, registries, modules, faults | Standard library |
| pcrecomp::win32_time / pcrecomp::win32_fs | Guest time/file ABI marshalling | bridge + win32compat |
| [win32compat::kernel32](../win32compat/README.md) | Selected Windows time/file/heap semantics | pcmodern |
| [pcmodern::core](../pcmodern/README.md) | Host storage, clock/sleep/log, filesystem/files | Host OS, standard library, Threads |
| Configured executable | PE loader, register adapters, diagnostics, SDL/DXVK integration | Libraries + platform dependencies |

Generated time/file calls use ABI adapters; heap calls use the project memory
binding. Other APIs still use project shims. Generic libraries contain no
original application logic or revision-specific addresses.

## Memory and execution

The bridge borrows storage through `pc_bind_memory`. Checked translation
validates width/range; explicit reads/writes are little-endian.
pcmodern owns host allocation, while the project owns guest layout and lifetime.
Raw generated MEM accesses still bypass some checked helpers.

`PcContext` contains integer registers and flags. Existing x87/SIMD/segment
state and generated globals remain outside it. Do not claim full state transport
for arbitrary callbacks or multithreading.

The project dispatch observer retains names, coverage and frontier state.
Registered invocation delegates to libpcrecomp; successful generated lookups
still use generated tables. Unknown calls report faults and stop. Callbacks
require explicit signature marshalling; module registration is not a DLL loader.

## Boundaries and limits

The core bridge does not depend on a compatibility implementation; optional ABI
adapters provide that link. VFS roots/caches and host callbacks have global state.
Callbacks must outlive their installation; execution/registry access requires
serialization. Path normalization is not a security sandbox.

Windowing, messages, input, audio, threads and graphics remain partly in
`src/platform/`, `src/win32/` and `src/renderer/`.
Never migrate success stubs or title assumptions into generic libraries.
See [migration risks](runtime-architecture-audit.md) and
[heap layout](references/win32hle-integration.md).

## Evidence

Run `just test-runtime` for asset-independent checks and `just build` for the
game consumer. Source call chains and executable symbols establish actual use,
not complete semantics. Use `just frontier` for live execution observations.
Current counts and limitations are in the [README](../README.md#project-status).
Library tests do not prove initialization, rendering, gameplay or equivalence.
