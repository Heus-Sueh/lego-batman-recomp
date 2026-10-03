# Investigation 0004: Runtime Guest Memory & Import Resolution

> Documentation review (2026-09-19): **HISTORICAL**. Retain loader/import bring-up evidence. The diagram mixes RVAs and VAs; use the PE headers and current loader. Time, path lookup and bridge ownership have since changed.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## Overview

- **Investigation Date**: 2026-09-13
- **Phase Gate**: Milestone M4 (Runtime, Guest Memory & Import Resolution)
- **Objective**: Establish dynamic PE data section loading, Win32 IAT import resolution, VFS transparent file access, and indirect call logging.

---

## 1. Portable PE Data Section Loader (`SectionLoader`)

To allow recompiled x86-32 functions to read string constants, lookup tables, and initialized globals at their original compiled virtual addresses, `src/runtime/section_loader.cpp` parses the original game PE headers and copies data sections directly into the 2GB guest memory space:

```text
gta_sa.exe
   ↓
SectionLoader::load_pe_data_sections()
   ↓
GuestMemory (0x00400000 base)
├── 0x00458000: .rdata   (4,545,754 bytes copied) - strings, vtables, floats
├── 0x004A4000: .data    (262,144 bytes copied)   - initialized pointers
├── 0x0089E000: _TEXT_HA (68,226 bytes copied)    - auxiliary hooks
├── 0x008AF000: _rwdseg  (8 bytes copied)         - RW data segment
└── 0x008E4000: bss      (3,904,564 bytes)        - zero-initialized
```

### Verification
- Both `.rdata` and `.data` ranges pass runtime non-zero population checks.
- Code sections and DRM-encrypted sections are excluded from data loading.

---

## 2. Win32 Import Resolution Architecture

The `ImportResolver` (`src/win32/import_resolver.cpp`) provides a centralized registry mapping Windows DLL imports to cross-platform C++ implementations:

| Module | Resolved APIs | Implementation Strategy |
|---|---|---|
| **KERNEL32.dll** | `CreateFileA`, `ReadFile`, `WriteFile`, `CloseHandle`, `SetFilePointer`, `GetFileSize`, `GetFileAttributesA` | Routed through `VirtualFileSystem` for Linux case-folding |
| **KERNEL32.dll** | `timeGetTime`, `GetTickCount`, `QueryPerformanceCounter`, `QueryPerformanceFrequency` | Monotonic nanosecond clock via `std::chrono::steady_clock` |
| **KERNEL32.dll** | `VirtualAlloc`, `VirtualProtect`, `VirtualFree` | Coordinated with `GuestMemory` and standard allocator |
| **KERNEL32.dll** | `CreateMutexA`, `ReleaseMutex`, `GetCurrentProcessId`, `GetCurrentThreadId` | Thread-safe POSIX / Windows abstraction |
| **USER32.dll** | `CreateWindowExA`, `ShowWindow`, `DestroyWindow`, `AdjustWindowRectEx`, `SetWindowTextA`, `PeekMessageA` | Routed to host SDL3 platform layer |
| **WINMM.dll** | `timeGetTime` | SDL3 tick clock (`SDL_GetTicks`) |
| **vorbisfile.dll** | `ov_open_callbacks`, `ov_read`, `ov_clear` | Audio stream decoders |

---

## 3. Unresolved Indirect Call Safety & Logging

In accordance with project rules, unresolved calls never fail silently. When an indirect call targets an unmapped guest address:
1. `DispatchTable::dispatch()` detects the missing entry.
2. An error is output to stderr.
3. The full CPU register state (`EAX`, `ECX`, `EDX`, `EBX`, `ESP`, `EBP`, `ESI`, `EDI`), the calling address, and target address are recorded to `logs/unresolved_calls.log`.

**Sample Log Entry**:
```text
UNRESOLVED_CALL: caller=0x00748da0 target=0xdeadbeef context=unmapped_dispatch EAX=0x12345678 ECX=0x87654321 EDX=0x0 EBX=0x0 ESP=0x0 EBP=0x0 ESI=0x0 EDI=0x0
```

---

## 4. Automated Testing & Verification

Automated unit tests in `tests/test_runtime.py` run via `just test`:
- `test_01_binary_exists`: Executable artifact present.
- `test_02_headless_startup`: Clean bootstrap and shutdown.
- `test_03_section_loading_and_imports`: Verifies `.rdata` integrity and import resolution.
- `test_04_unresolved_indirect_call_logging`: Validates diagnostic logging to `unresolved_calls.log`.
- `test_05_recompiled_startup_dispatch`: Validates execution of recompiled startup slice.

All 5 tests pass in 0.014 seconds.
