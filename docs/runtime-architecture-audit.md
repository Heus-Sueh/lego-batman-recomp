# Runtime architecture audit

Status: VERIFIED by source inspection and baseline execution, 2026-09-19.
This audit precedes the runtime foundation migration. Existing unrelated working
tree changes are preserved.

| Component / current location | Responsibility and dependencies | Destination | Risk |
|---|---|---|---|
| `thirdparty/pcrecomp`, `scripts/analysis/lifter` | Analysis, instruction translation, generated templates | pcrecomp; revision-specific templates remain integration | High: generated ABI |
| `src/runtime/guest_types.h` | Registers plus GTA address constants | Registers: libpcrecomp; constants: game-specific | Medium |
| `src/runtime/guest_memory.*` | Guest arena, OS allocation, bump heap, GTA write tracing, generated global binding | Translation: libpcrecomp; allocation: pcmodern; heap/config/tracing: integration initially | High |
| `src/runtime/section_loader.*` | PE parsing and section mapping using GuestMemory | Future libpcrecomp module loader; retain current loader this phase | High |
| `src/runtime/dispatch.*` | Function registry, execution counters, frontier, file logging | Registry: libpcrecomp; frontier/legacy adapters remain integration | High |
| Generated dispatch and upstream recomp32 macros | Direct/indirect calls, globals, imports, fallback stubs | Stable libpcrecomp ABI incrementally; legacy ABI adapter in game | High |
| Callbacks in window/COM shims | Guest callbacks mixed with host API wrappers | Generic invocation: libpcrecomp; signature marshalling: win32compat/integration | High |
| `src/win32/import_resolver.*` | Named API bindings and native pointers | Generic import registry: libpcrecomp; binding catalog: win32compat | Medium |
| `src/win32/win32_timing.cpp` | SDL ticks and independent chrono QPC | win32compat -> pcmodern clock | Low; clock epochs must be explicit |
| `src/platform/vfs.*` | Root, path normalization, case lookup, cache | pcmodern host filesystem; Windows path semantics retained for now | Low |
| `src/win32/win32_fs.cpp` | Win32 flags, FILE handles, direct libc file I/O | Future win32compat -> pcmodern filesystem | Medium; preserve handle/error semantics |
| `src/win32/win32_threads.cpp` | Sleep and placeholder events/mutexes | Sleep: pcmodern; unimplemented synchronization stays explicit legacy debt | Medium |
| `src/win32/win32_memory.cpp` | Heap and Win32 memory semantics | Future win32compat using bridge memory | High |
| `src/platform/platform_sdl.cpp`, `win32_windowing.cpp` | SDL window/events/input and Win32 messages | Future host backend + win32compat; keep coupled code outside generic libraries | High |
| `win32_audio.cpp` | Audio/input compatibility placeholders | Future win32compat/backend, no expansion now | High |
| `win32_d3d9.*`, `src/renderer`, `src/rw` | DXVK, SDL, RenderWare, game presentation integration | Host/compat interfaces later; GTA middleware remains game-specific | High |
| `src/runtime/diagnostics.*` | Frontier and tracing, global application state | Application observer; new generic fault/log records in bridge/core | Medium |
| Generated bridge templates | Hardcoded game addresses, success stubs, synthetic initialization | Game-specific debt; never move into generic libraries | High |

## Baseline

Command: `just frontier` (one bounded headless frame).
Observed: PE entry, window creation, splash texture loading, presenter frame 1,
clean exit, 305 registered functions, 40 reached, 695 dispatches, zero reported
unresolved calls. Frontier last successful VA `0x00727420`, current
`0x00734e90`, no reported failure. These addresses are VERIFIED trace values.
The frame bound stops execution deliberately; it does not locate a later boot
blocker. A headless log claiming visual level 5 is not visual verification.
No M9/gameplay or equivalence claim follows from this baseline.

## Violations to contain

Existing generated code contains synthetic game behavior and includes project
classes. Existing fallback purges do not implement missing function behavior.
Registry logs count bridges as lifted functions. Some guest memory access is
unchecked. Pointer-to-integer conversions require distinguishing actual host
pointers from encoded guest handles. None of these is evidence of authentic
gameplay. This foundation will expose safe interfaces and migrate a bounded
working path, not copy these assumptions into reusable libraries.

## Reference

[N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime) separates
librecomp bridges from ultramodern services and accepts project callbacks for
platform I/O. Reuse that separation conceptually; x86 PE imports, little-endian
guest memory and Win32 semantics require distinct implementations. No source
code is copied from that project.
