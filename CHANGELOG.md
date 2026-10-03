# Changelog

All notable changes to the LEGO Batman: The Videogame PC Static Recompilation project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres strictly to **Recompilation, Not Recreation** (no fictional game logic stubs).

## [Unreleased] - 2026-09-27

### Added
- **Reconnaissance & Revision Initialization**:
  - Imported and analyzed clean unpacked retail PE32 `LEGOBatman.exe` (7.55 MB, MSVC 2005, TT Games Engine).
  - Configured `config/revisions/lego_batman/revision.toml` and `globals.toml`.
  - Disassembled and cataloged all 23,698 functions across the entire `.text` section into `config/revisions/lego_batman/functions.json`.
- **PE Loader & Memory Architecture**:
  - Enhanced `src/runtime/section_loader.cpp` to dynamically parse and map all 7 PE sections (`.text`, `.rdata`, `.data`, `.idata`, `.shr`, `CONST`, `.rsrc`).
  - Added dynamic binding and initialization for all 213 IAT import slots in guest memory (`memory.write32(slot_va, slot_va)`).
- **Platform & Host Compatibility Boundary**:
  - Implemented authentic Win32 host bridges in `scripts/analysis/lifter/dispatch_gen.py`:
    - `GetCurrentProcessId`, `GetCurrentThreadId`, `GetSystemTimeAsFileTime`
    - `GetProcessHeap`, `HeapAlloc`, `HeapFree` (allocating in guest `0x10000000+` range)
    - `SetThreadExecutionState`
    - `CoInitializeEx`, `CoInitialize`, `CoUninitialize`
    - `GetCurrentThread` (pseudo-handle `0xFFFFFFFE`)
    - `SetThreadAffinityMask`
    - `CoCreateInstance` (returns `REGDB_E_CLASSNOTREG` `0x80040154`, naturally activating the original fallback path)
    - `GetModuleFileNameA` (returns `"LEGOBatman.exe"`)
    - `MultiByteToWideChar` (ANSI/UTF-8 to UTF-16 conversion)
    - `SysAllocString`, `SysFreeString`
    - `MessageBoxA`
- **Authentic x86 Instruction Lifting (28 Functions)**:
  - CRT & SEH Startup: `mainCRTStartup` (`0x0073D6A3`), `__security_init_cookie` (`0x00747A53`), `__tmainCRTStartup` (`0x0073D4C3`), `_EH_prolog3` (`0x007453F0`), `_EH_epilog3` (`0x00745435`), `__security_check_cookie` (`0x0073CA0B`), `memset` (`0x0073CB50`).
  - WinMain Initialization Chain: `WinMain` (`0x00527F60`), `sub_005263B0` (screen query), `sub_005318A0` (CPUID & SSE feature detection), `sub_006D6810` / `sub_006E3C60` (global state), `sub_00526B20` & `sub_00526AD0` (command line argument parser and tokenizer populating `argc`/`argv`), `sub_00526EA0` & `sub_006D6800` (COM and subsystem initialization), `sub_00527EE0` (engine shutdown).
  - Main Game Engine Loop: `sub_00401668` (engine entry thunk), `sub_004B01D0` (TT Games Engine main simulation loop, 4,705 bytes).
  - Game Engine Loop Subsystems:
    - Audio mixer update: `sub_00642490`
    - Audio voice & channel updater: `sub_005818B0`
    - Audio hardware timer sync: `sub_0055E540`
    - Audio volume transition helper: `sub_005614E0`
    - Audio stream status updater: `sub_00580310`
    - Subsystem tick synchronizer: `sub_005F7B00`
    - Input device event poll & buffer flush: `sub_005360C0`
    - Frame delta & timer counter: `sub_005BDC00`
    - Memory cache trimmer: `sub_0070A490`

### Changed
- Removed legacy GTA San Andreas fake RenderWare structures (`RsGlobal`, `RwDevice`, `0x00C17050`, `0x00C8CF88`) from `dispatch_gen.py`.
- Updated `win32_process.cpp` to return `"LEGOBatman.exe"` in `shim_GetCommandLineA`.
- Configured CMake and Justfile for the native `legobatman` binary target.

### Verified (Level 3 Runtime Evidence)
- **Active Game Engine Simulation Loop Reached**:
  - The native static recompiled binary boots from `mainCRTStartup`, executes CRT setup, initializes SEH and security cookies, executes `WinMain`, detects CPU features (SSE/CPUID), parses command line arguments into `argc` and `argv`, initializes COM/platform boundaries, and enters `sub_004B01D0` (the main TT Games Engine simulation loop).
  - The engine runs in a continuous, active frame update loop (audio updates, input polls, frame delta updates, cache trimming) executing over 400,000 instructions/functions without crashes or faults.
