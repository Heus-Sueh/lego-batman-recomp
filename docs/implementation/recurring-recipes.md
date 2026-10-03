# Recurring Bring-Up Actions & Rapid Resolution Recipes

Status: active project guide, reviewed 2026-09-19. Start failures as `UNKNOWN`.
Use the [runtime architecture](../runtime-architecture.md) for ownership and
specialized skills for investigation methodology. Addresses below are historical
revision-specific leads, not evidence of function identity or ABI.

This document defines standardized workflows, patterns, and resolution recipes for GTA: San Andreas static recompilation (`pcrecomp`). Use this playbook to resolve execution frontier targets quickly without repeating redundant reverse-engineering steps.

---

## 1. Decision Matrix for Frontier Targets

When `just frontier` or `gta_sa --frames 5 --headless --coverage` reports an unresolved call (`ICALL: unresolved VA 0x...`), gather evidence before assigning the target to one of these candidate patterns:

| Pattern | Examples | Detection | Standard Resolution | Anti-Pattern to Avoid |
|---|---|---|---|---|
| **1. Standard CRT Function** | `malloc`, `free`, `_strupr`, `_fsopen`, `fopen`, `fclose`, `strlen`, `strcpy` | Function lives in CRT range (`0x820000+`), calls locale tables, heap handles, or file descriptors. | Use revision TOML for the mapping; implement validated external semantics in the appropriate bridge/compatibility layer. | Do not assume CRT identity or host ABI compatibility from an address range; investigate guest semantics. |
| **2. Win32 / OS API** | `GetModuleFileNameA`, `CreateFileA`, `DirectInput8Create`, `RegOpenKeyExA` | Call targets `0x858000+` (IAT) or standard COM entry point. | Use `libpcrecomp` import marshalling, `win32compat` semantics and `pcmodern` host services; inspect legacy `src/win32/` integration. Use 32-bit `HandleTable` for OS handles. | Never return raw 64-bit host pointers cast to `HANDLE` or `HINSTANCE`. |
| **3. DRM / Crack Trampoline** | `0x00530645` (`jmp 0x409abf`), `0x00541DD0` (`jmp 0x40380d`), `0x0052F590` | Jumps to `.HOODLUM` crack section or obfuscated stub (`0x401000..0x409000`). | Inspect the selected revision and actual control flow; prefer metadata or reusable lifting fixes, then preimage-checked patches. | Do not assume every instruction in a suspected wrapper is disposable. |
| **4. Switch / Jump Table** | `jmp [edx*4 + 0x52fe00]`, `jmp [eax*4 + 0x748ef8]` | Indirect jump inside function with contiguous cases in `.rdata` or `.text`. | **Lift as structured C++ `switch`** using upstream CFG recovery and bounds from PE analysis; project-specific recovery currently lives in `scripts/analysis/lifter/game_lifter.py`. | Never let indirect jump fallback to unhandled dynamic dispatch if cases are static. |
| **5. Authentic Game Logic** | `CGame::*`, `CLoadingScreen::*`, `CControllerConfigManager::*`, `Rs*` | Application logic inside `.text` (`0x401000..0x856000`). | **Add to `functions.toml`** (`lift = true`) with correct size, convention, and stack purge. | Never mock game state in synthetic C++ under `src/game/`. Recompile original binary code. |

---

## 2. Standard Recipes

### Recipe A: Adding a Lifted Function to `functions.toml`

1. **Inspect boundary**:
   ```bash
   just inspect <target_va>
   ```
2. **Determine attributes**:
   - Function size in bytes.
   - Calling convention: `cdecl`, `thiscall`, `stdcall`, `fastcall`.
   - Stack purge: derive bytes from reachable epilogues and call sites. Do not
     infer cleanup from argument count alone; arguments can span multiple words.
3. **Append entry** to `config/revisions/gta_sa_compact/functions.toml`:
   ```toml
   [[function]]
   rva = 0x00XXXXXX
   name = "Subsystem_FunctionName"
   size = <size_in_bytes>
   convention = "cdecl" # or "thiscall" / "stdcall"
   stack_purge = 0      # bytes popped by ret N
   confidence = "unknown" # replace only after recording supporting evidence
   lift = true
   evidence = ["UNKNOWN: record binary identity, disassembly and runtime validation"]
   ```
4. **Recompile and cycle**:
   ```bash
   just cycle
   ```

### Recipe B: Adding a Portable CRT or Subsystem Bridge

1. Prefer the stable import bridge described in [runtime architecture](../runtime-architecture.md). For an existing legacy bridge, its generator currently lives in `scripts/analysis/lifter/dispatch_gen.py`. Preserve its verified stack cleanup:
   ```cpp
   static void bridge_FunctionName(void) {
       // cdecl: caller cleans arguments; callee pops return address pushed by RECOMP_ICALL
       uint32_t arg1 = MEM32(esp + 4);
       eax = ...;
       esp += 4; // pop return address
   }
   // For stdcall / thiscall with stack args: esp += 4 + purge_bytes;
   ```
2. Declare the revision mapping in `functions.toml` with `bridge`, `lift = false`,
   `stack_purge` and evidence. The generator emits lookup entries; do not add
   hardcoded switch cases. A bridge is appropriate for an external boundary,
   not an excuse to recreate original game behavior.

### Recipe C: Resolving a DRM / Crack Trampoline

1. When a function jumps into `0x00401000..0x0040A000` (`.HOODLUM` section):
   ```text
   mov eax, 0x0052F560
   add eax, 0x00000030
   inc eax
   jmp eax
   ```
2. Compute actual target address using the de-obfuscation formula:
   $$\text{Target} = \text{val1} + \text{val2} + 1$$
3. Verify the entire relevant instruction sequence and selected binary before
   changing control flow. Use revision metadata or a preimage-checked patch when
   required. This arithmetic example is historical, not a general rewriting rule.

### Recipe D: Lifting a Jump / Switch Table

1. Identify the table bounds in `.rdata` or `.text` from the indirect jump instruction (`jmp [index*4 + table_base]`).
2. Extract the consecutive case targets.
3. Replace the indirect jump instruction during lifting with a structured C++ `switch`:
   ```cpp
   switch (table_index) {
       case 0: goto case_0xXXXXXX;
       case 1: goto case_0xYYYYYY;
       default: break;
   }
   ```
4. Ensure all case target labels exist within the function's lifted scope.
