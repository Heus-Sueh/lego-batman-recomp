# Reusable PC runtime foundation

Status: VERIFIED foundation and migrated paths; incremental migration, not a
complete replacement of the legacy generated ABI. See the
[pre-migration audit](runtime-architecture-audit.md).

## Responsibilities and call flow

```text
                 pcrecomp
                    │
                    ▼
             generated code
                    │
                    ▼
              libpcrecomp
                    │
                    ▼
              win32compat
                    │
                    ▼
            PCModernRuntime
                    │
             host abstraction
                    │
       ┌────────────┼────────────┐
       ▼            ▼            ▼
      SDL         native        test
```

The diagram is the intended service call flow. The migrated clock path already
follows it; the native and test clock backends exist. SDL graphics/input/audio
remain in the application integration, not in these libraries.

| Target | Responsibility | Link dependencies |
|---|---|---|
| `pcrecomp::bridge` | C ABI, borrowed guest arena, function/import registries, callbacks, module descriptors, diagnostic records | Standard library only |
| `pcrecomp::win32_time` | x86 stdcall time import marshalling | bridge + win32compat kernel32 |
| `win32compat::kernel32` | GetTickCount, timeGetTime, QPC/QPF, Sleep semantics | pcmodern core |
| `pcmodern::core` | Host memory, clock/sleep/log callbacks, root/path lookup | Standard library, host OS, Threads |
| `gta_sa` | Revision, PE loader, legacy ABI adapters, middleware/renderer integration | Required generic targets plus existing SDL/DXVK |

The core bridge does not link a particular compatibility implementation.
Compatibility handlers are registered callbacks. The optional time adapter
supplies the downward dependency without creating a reverse dependency from
win32compat to libpcrecomp. No generic target links GTA, SDL or the code generator.
`runtime_dependency_rules` checks source includes; isolated builds exercise the
link boundary without the game, proprietary files, SDL or DXVK.

## Generated-code ABI v1

Public headers live under `libpcrecomp/include/pcrecomp/` and compile as C.
`pc_guest_addr_t` is always 32 bits; host pointers are never truncated into it.
`PcContext` carries integer registers and EFLAGS. It is not yet a full x86
machine state: legacy x87, SIMD, segments and local condition temporaries remain
with the existing generated ABI. Callers zero-initialize contexts.

Every API takes an explicit `PcRuntime*`; multiple isolated instances are
possible. Registries and guest execution are single-threaded: serialize access
externally. `PcRuntimeConfig` copies application/root strings; diagnostics and
handler user-data are borrowed and must outlive their registration. Root strings
are descriptive configuration; the application sets the filesystem root.

Functions return `PcStatus`. Unknown calls preserve the context, emit a complete
integer-register fault and return `PC_UNKNOWN_TARGET`. Callers must stop/propagate
failures, not treat them as guest return values. `PC_YIELDED` represents an
intentional host interruption; the GTA adapter uses it for its frame bound.
Handlers and diagnostic callbacks must not throw across the C boundary. The
library catches accidental guest handler exceptions as `PC_INTERNAL_ERROR`.
The GTA adapter explicitly transports legacy C++ exceptions back to its caller.

## Memory model

The bridge borrows an arena with `pc_bind_memory`; it does not allocate OS memory.
`pc_translate` checks an address and access width using subtraction to avoid
overflow. An unbound or out-of-range access returns failure. Reads/writes are
explicitly little endian, allow unaligned guest addresses and leave destination
values unchanged on failure. A translation is valid only until unbind/release.
Arena bounds do not implement per-page PE protection or unmapped-section checks.

`pcmodern::allocate_memory` owns OS allocation (VirtualAlloc or mmap), using
read/write storage: lifted native instructions live outside the guest arena.
GTA retains arena size, bump-heap placement, generated global binding and its
address-specific debug observer in `src/runtime/guest_memory.*`. Its translation
delegates to the bridge. Legacy generated MEM macros still use the raw arena;
replacing every legacy memory operation is outside this bounded foundation.

## Dispatch, ICALLs, callbacks and modules

`pc_register_guest_function` binds a guest address to a handler and opaque data.
`pc_call_guest`, `pc_call_indirect`, and `pc_invoke_guest_callback` share the
resolver. Duplicate registration is rejected. Callbacks require callers to
marshal the correct guest signature/stack; this is not an automatic native
callback trampoline or a threading implementation.

GTA's existing `DispatchTable` retains names, coverage and execution-frontier
observation while delegating invocation to this registry. Its adapters execute
legacy global-register functions; they do not yet synchronize a complete x86
state for arbitrary multithreaded callbacks.

Legacy RECOMP_ICALL/ITAIL successful lookups still use the generated tables.
Their unknown-target branches now call the generic diagnostic hook and stop,
instead of returning zero or popping the stack and continuing. The generic
library reports caller, target and CPU state. The legacy adapter has no unified
EFLAGS global and reports zero for that field; full flag capture remains open.
The compatibility return-address sentinel remains a recognized tail return.
Declared auto-purge stubs remain declared stubs, not implementations of behavior.

`pc_register_module` records a name, base, size and entrypoint, rejects overlap
and 32-bit range overflow, and copies the name. `pc_find_module` returns the
containing module. The GTA loader registers its actual PE optional-header
values after loading. This is a module registry, not a new PE/DLL loader.

## Import and Win32 path

The four generated time bridges now use:

```text
legacy generated register globals
 -> GTA ABI adapter (snapshot integer registers)
 -> pc_call_import
 -> pcrecomp::win32_time (guest stdcall marshalling and bounds checks)
 -> win32compat::kernel32
 -> pcmodern host clock
```

Import IDs are adapter-local API identifiers, never GTA VAs. QPC/QPF validate
guest stack/output memory and write a 64-bit little-endian result. A null output
pointer returns API failure. Stack cleanup includes the guest return address.
Other generated Win32 bridges still use legacy shims and need incremental
migration; the entire generated source is not yet dependency-free.

The old `win32_timing.cpp` now contains forwarding wrappers only; timing semantics
have one implementation. `Sleep` forwards through win32compat to the host sleep
callback. Placeholder event/mutex APIs stay outside generic libraries.

## Host services and filesystem boundary

`pcmodern::HostCallbacks` can replace time, sleep and categorized logging.
Install them before execution and keep user-data alive. Callback selection is
mutex-protected; invocation happens outside that mutex. Replacing the backend
does not synchronize the lifetime of old callback user-data. Native time is
monotonic nanoseconds with an unspecified epoch. All migrated Win32 time APIs
share it; QPF is 1 GHz, tick APIs wrap at 32 bits. Previously SDL ticks and QPC
used different epochs; absolute tick values are therefore not preserved.

The existing root, separator normalization, case-insensitive path lookup and
cache moved to `pcmodern::VirtualFileSystem`. `src/platform/vfs.h` is a forwarding
alias. No second path resolver remains. Windows-path compatibility behavior is
preserved, including its limitations; the root is not a security sandbox.
Win32 file handles/flags, direct libc file operations, save/mod policies and the
remaining threading services have not been migrated. No streaming/game logic
was moved into pcmodern.

## Decisions and rejected alternatives

| Choice | Reason | Alternative rejected / reason |
|---|---|---|
| Explicit runtime instances and status-returning C API | Portable generated surface and independent tests | Global-only API: couples consumers and hides failures |
| Borrow arena, allocate in host core | Keeps guest semantics independent of OS APIs | OS allocation in bridge: mixes execution and host responsibilities |
| Optional time adapter target | Win32 semantics can build without generated ABI | Mutual library linkage: creates cycles |
| Migrate clock and VFS first | Existing bounded, testable behavior | Move whole Win32/graphics layer: imports game assumptions and expands scope |
| Preserve legacy adapters | Maintains current code generator and frontier | Full context/IR rewrite: separate task and excessive regression risk |
| Explicit unknown-call failures | Prevents fake-success execution | Zero-return fallback: hides missing behavior |
| No generic SDL dependency | Native and deterministic backends build independently | Move SDL platform wholesale: includes guest/game integration |

The separation is inspired by
[N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime)'s bridge/service
split and callback-based I/O. Its console OS, memory and ABI semantics are not
copied into the x86/Win32 runtime.

## Validation and final migration report

VERIFIED on the current Linux host:

- `just test-runtime`: three CTest suites pass without retail files or graphics.
  Covers 8/16/32-bit memory, unaligned/endian accesses, bounds/overflow, known and
  unknown calls, fault registers, callback success/failure, modules, C linkage,
  synthetic clock import flow, 32-bit clock wrapping, QPC/QPF and stack cleanup,
  OS allocation, case lookup, cache reset and dependency direction.
- Negative cases require failures for invalid memory/calls/imports, duplicate
  mappings and throwing handlers. `legacy_icall_failure` ensures ICALL/ITAIL do
  not continue or mutate the guest result/stack on unknown targets.
  A deliberate clock-divisor mutation (2,000,000 instead of 1,000,000) made the
  C-caller import assertion fail. The mutation was reverted and the suite passed.
- `just lift`, `just build`, and CTest in the game build pass.
- `just test`: 20/20 existing tests pass with graphical-session access. Inside
  the sandbox three video-dependent tests fail with no available video device;
  assertions were not weakened.
- `just frontier` after each migration preserves the baseline: 305 registered,
  40 reached, 695 dispatches, zero reported unresolved calls, last successful
  `0x00727420`, current `0x00734e90`, clean bounded exit.
- `gta_sa --frames 300 --headless`: reaches presenter frame 300 and exits 0.
  Counters are diagnostic markers, not proof of authentic gameplay or pacing.

Architecture before: one application target owned runtime, platform services,
Win32 semantics, generated bridge implementations and game integration.

Architecture after: independent bridge/core/Win32 targets, optional time import
adapter and a game integration that consumes them. Full legacy ABI migration
remains explicitly incomplete.

Added directories: `libpcrecomp/`, `pcmodern/`, `win32compat/`, `tests/runtime/`.
Added integration: `src/runtime/bridge_adapter.*`. Added this document and audit.
Migrated files: VFS implementation/header into pcmodern with a compatibility
alias; extracted OS arena allocation and time semantics into generic layers.
Removed duplicate implementation: old `src/platform/vfs.cpp`; its maintained
implementation is preserved in `pcmodern/src/filesystem.cpp`. Other legacy
wrappers remain for existing call sites. No proprietary artifacts are added.

Generic code extracted: arena allocation, address translation contract,
integer context, registry execution, module metadata, clock/sleep semantics,
root/path resolver. Remaining GTA-specific code: PE revision policy, heap
layout, game-address diagnostics, RenderWare glue and historical generated
compatibility stubs.

Regressions: none observed in the recorded frontier, 300-frame bound or existing
suite with display access. Windows/macOS runtime behavior is UNKNOWN: no Windows
cross-compiler or corresponding execution host was available. Existing game
CMake still assumes a Linux DXVK shared object; the isolated generic build does
not have that dependency.

Highest demonstrated game validation: Level 4 initialization and observed
presentation calls. Tests/logs alone do not establish visual correctness, M9,
or equivalence. The foundation affects memory, dispatch and import subsystems;
no milestone gate is promoted by this refactor.

Violations remaining: legacy raw MEM accesses and global registers; project
headers in legacy generated bridges; synthetic game initialization and purge
stubs; mixed coverage labels; unreviewed pointer/handle conversions in other
shims. File handle casts are often encoded integer IDs, while generated return
pointer casts need per-API review; no blanket pointer-truncation fix was applied.

Recommended next task: move one additional real file API path through the
stable import bridge and host filesystem I/O, retaining guest handle semantics
and adding independent error/lifetime tests. Then migrate successful legacy
ICALL lookup tables and complete context synchronization incrementally.
