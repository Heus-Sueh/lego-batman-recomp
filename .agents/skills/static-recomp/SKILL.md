---
name: static-recomp
description: "Lift x86 instructions to C/C++, reconstruct control flow graphs, recover jump and switch tables, manage generated code boundaries, and configure deterministic binary patches for static recompilation."
---

# static-recomp: Static Code Lifting & C++ Generation

## 1. Purpose
Translate discovered x86 machine instructions and basic blocks into syntactically valid and behaviorally equivalent C/C++ source code. Manage the code generation pipeline, switch table transformations, and deterministic patch configuration.

## 2. When to Use
- When lifting newly discovered functions from the binary into the recompilation build.
- When transforming unhandled indirect jumps into structured C++ switch statements.
- When configuring deterministic preimage-checked patches for unliftable instructions.
- Do NOT use for manual implementation or rewriting of game logic (prohibited).
- Do NOT use for reverse engineering binary structures or ABIs (use `$binary-analysis`).
- Do NOT use for runtime crash debugging (use `$recomp-debug`).

## 3. Inputs
- Disassembly database and verified function metadata from `functions.toml`.
- Static recompilation lifter tool (`lift32.py` or project lift driver).
- Target output directory for generated C++ (`generated/recomp/`).
- Deterministic patch configuration (`patches.toml`).

## 4. Workflow

### Step 1: Function Selection & Pre-Conditioning
1. Select candidate functions on the active execution frontier.
2. Verify function metadata in `functions.toml`: RVA, size, calling convention, and stack purge.
3. Build the function's dependency closure: direct callees, tail jumps, indirect
   targets, import slots, jump-table data, and ABI cleanup requirements. Each
   dependency must be lifted, explicitly bridged, or left as a visible unresolved
   diagnostic; do not silently no-op it.
4. Search runtime generators and maintained source for an existing hand-written
   body owning the same VA. Remove or route the duplicate only after confirming
   that the lifted function covers the behavior; never leave two competing owners.

### Step 2: Lifter Execution
1. Run the instruction lifter against the target function:
   ```bash
   just lift
   ```
   Use the repository's canonical lift driver or equivalent so revision metadata,
   discovery, dispatch generation, and output partitioning remain consistent.
2. The lifter decodes instructions, tracks guest register states (`eax`, `ecx`, `edx`, `ebx`, `esp`, `ebp`, `esi`, `edi`, flags), and emits structured C++ functions.

### Step 3: Jump Table & Switch Code Generation
When a function contains an indirect jump (`jmp [reg*4 + table]`):
1. Extract case target addresses from the binary's read-only data section.
2. Emit a structured C++ `switch` statement replacing the raw indirect jump:
   ```cpp
   switch (table_index) {
       case 0: goto case_0xXXXXXX;
       case 1: goto case_0xYYYYYY;
       default: break;
   }
   ```
3. Verify that all case targets map to valid basic block labels within the lifted function.

### Step 4: Deterministic Patch Management (Last Resort)
If a function contains self-modifying code, illegal opcodes, or unliftable constructs:
1. Prefer fixing the lifter or revision metadata first.
2. If a patch is strictly necessary, define a preimage-checked patch in `patches.toml`:
   ```toml
   [[patch]]
   rva = 0x00XXXXXX
   expected_sha256 = "<sha256_of_original_bytes>"
   replacement_bytes = "<hex_string>"
   reason = "Bypass unliftable anti-debug timing loop"
   ```
3. The lifter applies the patch only if the source bytes match `expected_sha256`.

### Step 5: Compilation & Link Verification
1. Build the generated C++ files alongside the runtime.
2. Verify that there are no syntax errors, undefined register variables, or invalid label jumps.

### Step 6: Targeted Runtime Verification

1. Regenerate from a clean metadata/lifter invocation; never validate a manually
   edited generated artifact.
2. Execute the lifted function through the production dispatch path with controlled
   inputs when feasible.
3. Check exact stack restoration for its calling convention and one meaningful
   output (register, memory transition, callback, or trace).
4. Run the current execution-frontier regression to detect behavior lost outside
   the targeted function.

## 5. Evidence Rules
- Generated code must strictly correspond to original binary bytes.
- Never manually edit files under `generated/recomp/`. All changes must originate from lifter improvements, revision metadata, or deterministic patches.
- Maintain status: `LIFTED`, `COMPILED`, `RUNTIME_VERIFIED`, or `DIVERGED`.
- `LIFTED` proves source provenance, not runtime behavior. `COMPILED` proves only
  build compatibility. Use `RUNTIME_VERIFIED` only for the exact path and inputs
  exercised by a reproducible probe or natural execution trace.

## 6. Validation
- **Level 1 (Static)**: Generated source files compile cleanly under target compilers (Clang, GCC, MSVC) with zero warnings.
- **Level 2 (Build)**: Output objects link successfully into the native executable.
- **Level 3 (Targeted Runtime)**: Dispatch reaches the function, preserves its ABI,
  and produces an exact expected observable transition.
- Re-run generation and verify ownership guards so a synthetic duplicate cannot
  mask or replace the lifted implementation.

## 7. Failure Handling
- If generated C++ fails to compile due to missing labels: verify basic block boundaries and ensure jump table cases do not branch outside function scope.
- If lifter fails on an unknown x86 opcode: implement the opcode semantics in the upstream lifter (`thirdparty/pcrecomp/`) rather than hacking the output.

## 8. Anti-Patterns
- **Hand-Editing Generated Code**: Modifying files inside `generated/recomp/`. Any manual edits will be overwritten on the next generation cycle.
- **Speculative Lifting**: Lifting thousands of unverified functions en masse before earlier startup functions are validated.
- **Unchecked Patches**: Applying binary patches without SHA-256 preimage verification.
- **Synthetic Shadowing**: Keeping a hand-written function with the same VA after
  adding an authentic lifted implementation.
- **Dependency-by-No-Op**: replacing a callee/import with success solely so the
  parent function appears to execute.

## 9. Knowledge Capture
When code generation completes:
- Update function status in `functions.toml` (`lift = true`).
- If custom lifting patterns (e.g. switch table regex or post-processors) were created, record them in `docs/implementation/`.

## 10. Completion Criteria
- Target function is lifted into `generated/recomp/`.
- Generated code compiles cleanly as part of the project build.
- Function is registered in the runtime function lookup table.
- No competing manual body owns the same VA.
- ABI cleanup and one meaningful runtime effect are verified, or the function is
  explicitly reported as only `COMPILED` with the missing runtime evidence.
