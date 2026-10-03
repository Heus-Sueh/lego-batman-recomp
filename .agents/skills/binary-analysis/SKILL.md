---
name: binary-analysis
description: "Analyze x86 PE binaries, extract sections, perform recursive-descent disassembly, determine function boundaries, and verify x86-32 calling conventions (cdecl, stdcall, thiscall, fastcall) and stack purge rules from raw binary evidence."
---

# binary-analysis: x86 PE Disassembly & Calling Convention Discovery

## 1. Purpose
Analyze legacy x86 PE32 executables using disassembly and static binary analysis. Extract structural metadata, identify function candidates, discover jump tables, and determine accurate calling conventions and stack purge parameters from instruction-level evidence.

## 2. When to Use
- When analyzing a new target executable revision.
- When inspecting an unclassified address discovered during execution.
- When verifying the calling convention, parameter count, or stack cleanup of a function.
- When recovering indirect jump targets or switch tables from binary data.
- Do NOT use for instruction lifting or C++ code generation (use `$static-recomp`).
- Do NOT use for runtime crash triage or frontier stepping (use `$recomp-debug`).

## 3. Inputs
- Target executable binary file path.
- PE analysis metadata (sections, image base, entry point, relocations, export/import directory).
- Target virtual address (VA) or relative virtual address (RVA) to inspect.
- Disassembly tools (`disasm32.py`, objdump, Ghidra/IDA headless exports).

## 4. Workflow

### Step 1: PE Structural Inspection
1. Verify executable headers: machine architecture (`IMAGE_FILE_MACHINE_I386`), `ImageBase`, `AddressOfEntryPoint`, `Subsystem`.
2. Inspect section table: map executable sections (`.text`, `CODE`) and read-only data sections (`.rdata`).
3. Check for section encryption or packing (entropy > 7.2 indicates packed/encrypted code requiring an unpacked image).

### Step 2: Recursive-Descent Disassembly
1. Disassemble from validated entry points or discovered call targets.
2. Follow direct control-flow instructions (`call`, `jmp`, `jcc`).
3. Delineate function boundaries by identifying prologues (`push ebp; mov ebp, esp` or frame setup) and epilogues (`leave; ret` or `ret N`).

### Step 3: Calling Convention & Stack Purge Verification
Never trust decompiler syntax alone. Inspect raw assembly instructions:

| Convention | Epilogue Instruction | Call Site Sequence | Register Arguments |
|---|---|---|---|
| `cdecl` | `ret` (or `ret 0`) | `add esp, 4*N` immediately after `call` | None |
| `stdcall` | `ret N` ($N = 4 \times \text{args}$) | No `esp` adjustment after `call` | None |
| `thiscall` | `ret N` ($N = 4 \times \text{stack\_args}$) | No `esp` adjustment after `call` | `ecx` = `this` pointer |
| `fastcall` | `ret N` | No `esp` adjustment after `call` | `ecx` (1st), `edx` (2nd) |

1. **Check the Epilogue**:
   - `ret` without immediate operand indicates caller cleanup (`cdecl`), UNLESS the function has 0 arguments.
   - `ret N` indicates callee cleanup of $N$ bytes (`stdcall` or `thiscall`).
2. **Correlate with Call Sites**:
   - Check at least 2 distinct call sites. Does the caller adjust `esp` after the call?
3. **Check Register Preconditioning**:
   - Is `ecx` loaded with an object pointer before the call (`mov ecx, esi` or `lea ecx, [var]`) without being pushed? If yes, it is `thiscall`.

### Step 4: Jump Table & Switch Analysis
1. Detect indirect jump patterns: `jmp [reg*4 + table_base]`.
2. Determine case bounds: look for unsigned comparisons (`cmp reg, max_cases; ja default_label`) preceding the jump.
3. Read contiguous 32-bit pointers from `.rdata` starting at `table_base` up to `max_cases`.

## 5. Evidence Rules
- Label conclusions with strict evidence levels:
  - `VERIFIED`: Confirmed by epilogue `ret N` AND multiple call site `esp` checks AND runtime trace.
  - `PARTIALLY_VERIFIED`: Confirmed by epilogue or call sites, but not both.
  - `INFERRED`: Suggested by decompiler heuristics or naming conventions without instruction proof.
  - `UNKNOWN`: Insufficient evidence to establish convention.
- Never record a calling convention as fact without stating the epilogue instruction and purge count.

## 6. Validation
- Cross-check stack purge count against parameter count: $\text{stack\_purge} = 4 \times \text{stack\_param\_count}$.
- Ensure variable argument functions (`...`) are strictly marked `cdecl`.

## 7. Failure Handling
- If disassembly hits illegal opcodes, check if the address is inside an encrypted section, padding/alignment bytes (`0xCC` or `0x90`), or embedded jump table data.
- If stack adjustment at call site conflicts with `ret N`, check if the function is a tail-call jump (`jmp`) or varargs.

## 8. Anti-Patterns
- **Decompiler Blind Faith**: Accepting Ghidra/IDA `__cdecl` or `__stdcall` output without inspecting machine instructions.
- **Silent Defaults**: Defaulting unknown functions to `cdecl` with 0 purge when callee pops arguments.
- **Treating Data as Code**: Attempting to disassemble `.rdata` jump tables or string pools as executable code.

## 9. Knowledge Capture
When this skill completes:
- Record discovered functions with RVA, size, convention, and stack purge in project revision manifests (`functions.toml`).
- If a new calling convention edge case or pattern is identified, document it in `docs/gotchas/`.

## 10. Completion Criteria
- Function candidate has identified start RVA, size in bytes, verified calling convention, and stack purge count.
- Evidence level is explicitly stated (`VERIFIED` or `PARTIALLY_VERIFIED`).
