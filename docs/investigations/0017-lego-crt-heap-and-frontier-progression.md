# Investigation: 0017 - LEGO Batman Authentic CRT Heap Initialization and Frontier Advancement

## Revision
- **Date / source revision**: 2026-10-03, master working tree
- **Executable**: `LEGOBatman.exe` (v1.0 GOG Clean, PE32)
- **Milestone / gate**: M1 / Gate G1 (CRT Startup & Authentic Program Entry)
- **Document status**: Active investigation
- **Evidence status**: VERIFIED (Targeted runtime trace, instruction disassembly, caller stack inspection)

## Execution Frontier
- **Prior Failing VA**: `0x0073D4C3` (__tmainCRTStartup calling `0x0074797F`)
- **Resolved Target VA**: `0x0074797F` (`_heap_init`)
- **New Failing VA**: `0x0073D4C3` (__tmainCRTStartup calling `0x00745056`)
- **New Target VA**: `0x00745056` (`_mtinit` / TLS & FLS thread subsystem initialization)
- **Failure**: Unmapped indirect call to `0x00745056`

## Classification
- [x] Recompiler / Revision Metadata (discovered authentic CRT initialization functions `_heap_init`, `_heap_select`, `_get_osplatform`, `_get_winmajor`)
- [x] Runtime / Win32 Compatibility (`HeapCreate` and `HeapDestroy` import shims and dispatch bridges)

## Evidence
1. `scripts/tools/recomp_tool.py`:
   - Identified and fixed bug in `analyze_function_binary` where `ins.group(capstone.CS_GRP_CALL)` caused inspection to treat forward inter-procedural function calls as intra-function branch targets, inflating candidate size to thousands of instructions and overrunning into subsequent functions.
   - Identified and fixed bug where `pop ecx` was treated as reading ECX rather than writing, causing inaccurate `thiscall` inferences.
   - Verified exact 90-byte boundary for `0x0074797F` (ends with `ret` at `0x007479D8`, immediate next instruction is `push ebp` at `0x007479D9`).
2. Calling convention verification for `0x0074797F`:
   - Epilog: plain `ret` at `0x007479A1` and `0x007479D8`.
   - Caller `0x0073D594` pushes 1 argument (`push ebx`) and cleans up the stack with `pop ecx` at `0x0073D599`.
   - Calling convention: `cdecl`, stack purge: 0 bytes.
3. Import bridges:
   - Added `HeapCreate` (`0x02A16A44`, purge 12) and `HeapDestroy` (`0x02A16A40`, purge 4) to `KNOWN_IMPORT_BRIDGES` in `dispatch_gen.py`.
   - Added `shim_HeapCreate` and `shim_HeapDestroy` to `win32_shims.h`, `win32_memory.cpp`, and `import_resolver.cpp`.
4. Dependency closure lifted:
   - `0x0074797F`: `_heap_init` (90 bytes, `cdecl`, purge 0)
   - `0x00747924`: `_heap_select` (91 bytes, `cdecl`, purge 0)
   - `0x0073E927`: `_get_osplatform` (55 bytes, `cdecl`, purge 0)
   - `0x0073E9D6`: `_get_winmajor` (60 bytes, `cdecl`, purge 0)
5. Runtime Execution Trace:
   ```text
   ICALL: unresolved VA 0x00745056 from 0x0073D4C3
   [icall] status=3 caller=0073d4c3 target=00745056 eax=00000001 ecx=00000001 edx=00000001 esp=002fff7c ebx=00000001 ebp=002ffffc esi=00000002 edi=00000a28 eflags=00000000
   ```
   - EAX returned by `_heap_init` is `0x00000001` (success).
   - `__tmainCRTStartup` branched via `jne 0x73d5a6` past error handler `0x73d59e` to `0x73d5a6`.
   - Runtime reached functions increased from 5 (10.2%) to 9 (17.0%).
   - Total function dispatches increased from 6 to 10.

## Progression: `_mtinit`, `_initterm`, `_ioinit`, `_get_environ`

### 1. `_mtinit` & CRT Multithreading Subsystem
- Lifted `_mtinit` (`0x00745056`, 388 bytes, `cdecl`, purge 0) and its complete closure:
  - `_calloc_crt` (`0x0074524B`, 72 bytes, `cdecl`, purge 0)
  - `calloc` (`0x0074186C`, 286 bytes, `cdecl`, purge 0, with SEH frame)
  - `_malloc_crt` (`0x0074520B`, 64 bytes, `cdecl`, purge 0)
  - `malloc` (`0x007410D2`, 195 bytes, `cdecl`, purge 0)
  - `free` (`0x00740F1B`, 142 bytes, `cdecl`, purge 0)
  - `memset` (`0x0073CB50`, 122 bytes, `cdecl`, purge 0)
  - `_mtinitlocks` (`0x0074C0E7`, 73 bytes, `cdecl`, purge 0)
  - `_InitializeCriticalSectionEx` (`0x0074C677`, 197 bytes, `cdecl`, purge 0)
  - `_lock` (`0x0074C275`, 49 bytes, `cdecl`, purge 0)
  - `_unlock` (`0x0074C185`, 21 bytes, `cdecl`, purge 0)
  - `_initptd_locks` (`0x00744D72`, 182 bytes, `cdecl`, purge 0)
  - `_initptd_sub` (`0x0075023D`, 134 bytes, `cdecl`, purge 0)
  - `_unlock_locale_sub` (`0x00744E28`, 9 bytes, `cdecl`, purge 0)
  - `_getptd_noexit_real` (`0x00744E31`, 119 bytes, `cdecl`, purge 0)
  - `_getptd_wrapper` (`0x00744CEA`, 50 bytes, `cdecl`, purge 0)
  - `_errno` (`0x0073FD74`, 19 bytes, `cdecl`, purge 0)
  - `_amsg_exit` (`0x0073E87E`, 36 bytes, `cdecl`, purge 0)
  - `_mtterm` (`0x0074C130`, 85 bytes, `cdecl`, purge 0)
  - `_mtinitlocknum` (`0x0074C1B2`, 186 bytes, `cdecl`, purge 0)
  - `_unlock_10` (`0x0074C26C`, 9 bytes, `cdecl`, purge 0)
- `_mtinit` completed with `EAX = 1` (success), successfully allocating the per-thread data via `HeapAlloc` on `_crtheap` and storing the thread ID in TLS.

### 2. `_initterm` Table Loop
- Lifted `_initterm` (`0x007478DC`, 36 bytes, `cdecl`, purge 0).
- Iterates `0x9306B0` to `0x9307B0` (`__xi_a` to `__xi_z`). In `LEGOBatman.exe`, this table is all-zero, so the function iterates cleanly and returns.

### 3. `_ioinit` & Standard Streams
- Lifted `_ioinit` (`0x00747650`, 576 bytes, `cdecl`, purge 0).
- Added Win32 import bridges:
  - `GetStdHandle` (`0x2A16A20`, stdcall, purge 4)
  - `GetFileType` (`0x2A16A3C`, stdcall, purge 4) -> returns 2 (`FILE_TYPE_CHAR`)
  - `SetHandleCount` (`0x2A16A38`, stdcall, purge 4) -> returns requested count
- `_ioinit` allocates stream descriptors with `_calloc_crt`, configures standard handles 0, 1, 2, initializes critical sections, and returns `EAX = 0` (success).

### 4. `_get_environ`
- Lifted `_get_environ` (`0x0074751B`, 309 bytes, `cdecl`, purge 0).
- Added Win32 import bridges:
  - `GetEnvironmentStringsW` (`0x2A16A34`, stdcall, purge 0) -> populates guest memory at `0x00030000` with double-null terminated wide string.
  - `FreeEnvironmentStringsW` (`0x2A16A2C`, stdcall, purge 4) -> returns 1.
  - `WideCharToMultiByte` (`0x2A16A30`, stdcall, purge 32) -> converts UTF-16LE to ANSI.
- `_get_environ` allocates environment block on `_crtheap` (returning pointer `0x100009C0`), which is stored in `_aenvptr` (`0x2A10AA8`).

### 5. Final CRT Startup & Static Initializers (`.CRT$XCU`) Milestone

- **Static Constructor Table (`.CRT$XCU`)**:
  - Spanned `0x00850104` to `0x00850278` (93 non-zero entries).
  - Lifted and verified all remaining static initializers up to `0x0084CA20`.
  - Discovered and fixed critical Capstone lifter handling for AMD-optimized `repz ret` (`f3 c3`) instructions in `pcrecomp` (`disasm32.py`, `lift32.py`, `generate.py`), preventing spurious cookie failure fallthroughs.
  - Lifted constructor loop primitives (`__ehvec_ctor` `0x0073D235`, purge 20; custom vector ctor `0x004087A0`, purge 16).
  - Corrected dual-return function bounds for `sub_006F3BA0` (120 bytes) and `sub_00526FF0` (139 bytes).
  - All 93 static initializers completed cleanly without stubs or mocks.

### 6. Entrance into WinMain & Game Engine (`sub_004B01D0`)

- `_cinit` returned `0` to `__tmainCRTStartup` (`0x0073D6A3`).
- `__tmainCRTStartup` fetched command-line arguments and called `WinMain` (`0x00527F60`).
- `WinMain` executed authentic host window/input initialization and called ILT thunk `0x00401668` into `sub_004B01D0` (TT Games Engine entry point).
- Engine entry `sub_004B01D0` began execution and called argument buffer initializer `0x006D5510`.

### 7. Current Metrics & Frontier
- **Functions registered**: 298 (up from 49)
- **Functions runtime reached**: 226 (75.8%, up from 5 / 10.2%)
- **Total function dispatches**: 2,909 (up from 6)
- **Current Frontier**: `0x004B01D0` (TT Games Engine entry point) calling target `0x006D5510`
- **Milestone Gate G1 (CRT Startup & Program Entry)**: **PASSED (100% AUTHENTIC)**
