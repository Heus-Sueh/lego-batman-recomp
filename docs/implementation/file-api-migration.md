# Synchronous file lifecycle migration

Status: implemented reusable slice. Historical acceptance results below concern
this boundary's original migration, not the current game's boot status. Current
counts and blockers are maintained in the root README.

Reusable vertical slice: `CreateFileA(OPEN_EXISTING, GENERIC_READ)` -> `ReadFile` -> `CloseHandle`, with an independent synthetic caller and fixture.
The legacy implementation in `src/win32/win32_fs.cpp` now forwards to `win32compat`, sharing the unified 32-bit handle table. Path resolution is provided by `pcmodern::VirtualFileSystem`. See [runtime architecture](../runtime-architecture.md).

## Ownership

- **`libpcrecomp` optional adapter (`pcrecomp::win32_fs`)**: Bounded guest strings/buffers, stdcall stack marshalling, output values and explicit diagnostic failures.
- **`win32compat` (`win32compat::kernel32`)**: Monotonic 32-bit file-handle table (`0x1000+`), access/disposition rules (`GENERIC_READ`, `GENERIC_WRITE`, `OPEN_EXISTING`, etc.), invalid handles, EOF semantics, and Win32 error codes (`ERROR_FILE_NOT_FOUND`, `ERROR_INVALID_HANDLE`).
- **`pcmodern` (`pcmodern::core`)**: Host open/read/write/seek/tell/close operations via `pcmodern::file_*` and resource lifetime safety.
- **Project integration**: `src/runtime/bridge_adapter.*` registers file import bindings via `pc_register_fs_imports()`, and `src/win32/win32_fs.cpp` forwards file shims through `win32compat` without maintaining duplicate handle tables.

## Acceptance Evidence

1. Recorded `just frontier` before changes.
2. Verified standalone test `runtime_file` in `tests/runtime/file_foundation.cpp`:
   - Opens, reads, seeks, and closes a temporary mixed-case fixture file through the C ABI.
   - Verifies read contents match the fixture byte-for-byte and updates guest output pointers (`lpNumberOfBytesRead`).
3. Tested negative controls:
   - Missing path -> `WIN32_INVALID_HANDLE_VALUE` (0xFFFFFFFF) and `ERROR_FILE_NOT_FOUND`.
   - Out-of-bounds guest buffer -> `PC_MEMORY_BOUNDS` failure.
   - EOF -> returns success with 0 bytes read.
   - Invalid and double-closed handles -> returns 0 and sets `ERROR_INVALID_HANDLE`.
   - Exact stdcall stack cleanup (`esp += 4 + arg_size`).
4. Handle lifetime verified: monotonic 32-bit descriptors prevent 64-bit host pointer truncation.
5. Reproduce boundary checks with `just test-runtime`. Use `just build`,
   `just test` and `just frontier` separately for current integration evidence;
   the original migration's boot counters do not apply to another revision.
6. Single handle table: legacy `HandleTable` in `src/win32/win32_fs.cpp` removed, eliminating duplicate namespace.
