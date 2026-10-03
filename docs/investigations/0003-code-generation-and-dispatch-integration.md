# Investigation 0003: Code Generation & Dispatch Integration

> Documentation review (2026-09-19): **HISTORICAL**. Retain the original dispatch integration record. Current generation uses scripts/analysis/lifter and batched units; raw MEM macros do not guarantee valid memory accesses.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## Overview

- **Investigation Date**: 2026-09-13
- **Phase Gate**: Milestone M3 (Code Generation / Targeted Function Lifting)
- **Objective**: Synthesize generated C++ recompiled functions and dispatch tables, integrate with the guest memory model and register file, and verify compilation and dispatch execution.

---

## 1. Code Generation Architecture (`run_lift.py`)

The code generation pipeline (`scripts/analysis/run_lift.py`) reads the revision function manifest (`config/revisions/gta_sa_compact/functions.toml`) and emits clean, strictly segregated C++ sources into `generated/recomp/`:

```text
functions.toml
      ↓
scripts/analysis/run_lift.py
      ↓
generated/recomp/
├── recomp_globals.cpp    # Global x86-32 register file & memory offset
├── recomp_funcs.h        # Forward declarations of recompiled functions
├── recomp_dispatch.cpp   # Function pointer table & DispatchTable registry
├── gta_sa_crt.cpp        # mainCRTStartup, _initterm
├── gta_sa_winmain.cpp    # WinMain, InitApp, MainWndProc
├── gta_sa_rw.cpp         # RsEventHandler, RwEngineInit, Open, Start, etc.
└── gta_sa_game.cpp       # CGame_Initialise, CGame_Process, CTimer_*
```

In accordance with project rules, `generated/recomp/` is completely machine-generated and gitignored.

---

## 2. Global Register Model & Guest Memory Translation

1. **Register State (`recomp_globals.cpp`)**:
   - Implements full general-purpose 32-bit registers: `g_eax`, `g_ecx`, `g_edx`, `g_ebx`, `g_esp`, `g_ebp`, `g_esi`, `g_edi`.
   - Implements x87 FPU stack: `g_st[8]`, `g_fp_top`, `g_fpu_cw`.
   - Flat segment bases: `g_fs_base`, `g_gs_base`.
2. **Memory Translation**:
   - `g_mem_base` is dynamically initialized by `pcrecomp::runtime::GuestMemory` upon allocating the 2GB guest memory space.
   - Macros from `thirdparty/pcrecomp/runtime/recomp32/recomp_types.h`:
     ```c
     #define ADDR(va)    ((uintptr_t)(uint32_t)(va) + g_mem_base)
     #define MEM32(addr) (*(volatile uint32_t *)ADDR(addr))
     ```
   - All memory accesses inside generated functions resolve directly into the valid guest address range.

---

## 3. Dispatch Integration

`recomp_dispatch.cpp` bridges the generated C functions with the project's native `DispatchTable`:

```cpp
void register_generated_recomp_functions() {
    auto& dt = pcrecomp::runtime::DispatchTable::instance();
    dt.register_function(0x008246EFu, bridge_func_008246EF, "mainCRTStartup");
    dt.register_function(0x00748DA0u, bridge_func_00748DA0, "WinMain");
    // ... all 18 discovered functions registered
}
```

---

## 4. Observable Execution Evidence (Validation Level 3 Entry)

Running `./out/build/linux-amd64-release/gta_sa --headless --test-dispatch`:

```text
============================================================
  Grand Theft Auto: San Andreas - PC Static Recompilation  
============================================================
[Init] Discovered game installation at: "game/GTA San Andreas"
[VFS] Root set to: "game/GTA San Andreas"
[GuestMemory] Allocated 2GB guest address space at host 0x7f455a000000
[Dispatch] Registered 18 lifted functions.
[Win32] Initializing compatibility layer shims.
[RenderWare] Interception layer initialized.
[Renderer] Initialized at Tier 1 (Decoupled simulation rate: 30 FPS)
[Test] Executing recompiled entry point dispatch (0x008246EF)...
[Recomp] Entering mainCRTStartup (0x008246EF)
[Recomp] Executed _initterm (0x00824588)
[Recomp] Entering WinMain (0x00748DA0)
[Recomp] Executed InitApp (0x00745080)
[Recomp] Executed RsEventHandler (0x00619B70)
[Recomp] Executed RwEngineInit (0x007F6F10)
[Recomp] Executed RwEngineOpen (0x007F6BE0)
[Recomp] Executed RwEngineStart (0x007F6D80)
[Recomp] Executed RwOsGetFileInterface (0x00801CF0)
[Recomp] Executed CGame_Initialise (0x0053BC20)
[Recomp] Executed CTimer_Initialise (0x005617A0)
[Recomp] Executed CGame_Process (0x0053E980)
[Recomp] Exiting WinMain (0x00748DA0)
[Recomp] Exiting mainCRTStartup with status 0
[Test] Dispatch execution completed successfully.
[Init] Headless verification passed.
[Renderer] Shutdown complete.
[RenderWare] Interception layer shutdown.
[Exit] Recomp runtime completed cleanly.
```

---

## 5. Milestone Assessment

- **Level 1 (Static)**: Generated C++ code in `generated/recomp/` is syntactically valid.
- **Level 2 (Build)**: Project compiles and links cleanly with Ninja + GCC 16 (0 warnings, 0 errors).
- **Level 3 (Entry)**: Recompiled binary dispatches into `mainCRTStartup`, executes CRT startup, enters `WinMain`, and traverses RenderWare and game initialization.
- Reaches Milestone **M3** and prepares **M4 / M5 / M6**.
