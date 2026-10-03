---
name: recomp-debug
description: "Debug static recompilation execution frontiers, diagnose crashes, classify failures using the 3-failure rule, triage stack imbalances, and resolve unlifted indirect calls (ICALLs)."
---

# recomp-debug: Execution Frontier Debugging & Failure Triage

## 1. Purpose
Diagnose runtime failures, crashes, hangs, and execution halts in statically recompiled binaries. Maintain and advance the execution frontier, isolate root causes using the 3-failure classification system, resolve indirect calls (ICALLs), and triage stack imbalances.

## 2. When to Use
- When runtime execution crashes, segfaults, hangs, or hits an unresolved call.
- When advancing the execution frontier past the current blocker.
- When diagnosing stack corruption or unexplained return address jumps.
- When profiling runtime stalls or frame presentation jitter.
- Do NOT use for general binary reverse engineering (use `$binary-analysis`).
- Do NOT use for lifting instructions (use `$static-recomp`).

## 3. Inputs
- Last successful virtual address (VA) and crashing/halting VA.
- Register context dump (`eax`, `ecx`, `edx`, `ebx`, `esp`, `ebp`, `esi`, `edi`, `eflags`).
- Stack memory slice around `esp`.
- Execution trace logs (`logs/unresolved_calls.log` or runtime coverage traces).

## 4. Workflow

### Step 1: Record Execution Frontier State
Record the exact frontier quad:
```text
Last Successful VA: 0x00XXXXXX
Current / Failing VA: 0x00YYYYYY
Failure Classification: [RECOMPILER | RUNTIME | GAME-SPECIFIC]
Target VA: 0x00ZZZZZZ
```

### Step 2: The Three Failure Classes & Golden Rule
Classify every failure before proposing any code change:

```text
                     FAILURE
                        │
                        ▼
             Is generated code wrong?
                  /           \
                yes            no
                │               │
                ▼               ▼
          RECOMPILER       Is expected guest
                             environment wrong?
                              /          \
                            yes           no
                            │              │
                            ▼              ▼
                         RUNTIME       GAME-SPECIFIC
```

1. **RECOMPILER**: Bad instruction lifting, broken CFG, inverted jump conditions, wrong register preconditioning, or missing stack purge.
   - *Fix*: Improve lifter rules in upstream recompiler or fix function metadata in revision manifest.
2. **RUNTIME**: Guest memory unmapped, Win32 API shim missing/incorrect, file mapping failure, callback dispatcher fault, or thread synchronization bug.
   - *Fix*: Improve generic runtime or Win32 compatibility layer.
3. **GAME-SPECIFIC**: Engine-specific callback format, unusual middleware, or custom binary packing.
   - *Fix*: Title-specific shim or hook (only after eliminating recompiler/runtime causes).

> [!IMPORTANT]
> **The Golden Rule**: *Would another x86 Windows game potentially encounter this exact problem?* If yes, solve it in the generic recompiler or portable runtime, never in game-specific logic.

### Step 3: Resolving Unresolved Indirect Calls (ICALLs)
When execution reports `ICALL: unresolved VA 0x...`:
1. **Identify the Caller**: Check disassembly at caller VA. Is it a function pointer, COM vtable call, or callback?
2. **Inspect the Target Address**:
   - Inside CRT range $\rightarrow$ Bridge to host CRT.
   - Inside Import Address Table (IAT) $\rightarrow$ Add or fix Win32 API shim.
   - Inside genuine game `.text` $\rightarrow$ Lift the target function via `$binary-analysis` and `$static-recomp`.
   - Obfuscated DRM crack trampoline $\rightarrow$ Compute destination statically and bypass.
3. **Verify Stack Purge for ICALL**:
   - Ensure the bridge or shim pops the synthetic return address pushed by `RECOMP_ICALL` (`esp += 4`), plus any callee-purged arguments.

### Step 4: Triaging Stack Imbalances
When a function crashes on `ret`:
1. Check `esp` value at entry vs `esp` before `ret`.
2. If `esp` is offset by 4, 8, or 16 bytes, a callee called by this function had a mismatched stack purge count.
3. Trace every call made by this function to find which callee has an incorrect `ret N` vs `ret` designation.

## 5. Evidence Rules
- Never speculate on crash causes without register and stack dumps.
- Mark findings with explicit evidence levels:
  - `VERIFIED`: Reproducible crash trace with localized instruction and register dump.
  - `PARTIALLY_VERIFIED`: Crash reproduced, but exact corrupted caller unconfirmed.
  - `INFERRED`: Hypothesis based on symptom pattern without stack trace proof.

## 6. Validation
- After applying the fix, run execution through the test harness.
- Confirm execution advances past the previously failing VA to a new, higher execution frontier.

## 7. Failure Handling
- If a fix causes an immediate crash elsewhere, check for stack imbalance introduced by the fix before investigating the new crash.
- If execution enters an infinite loop, check loop counter registers and flag update semantics in the lifted code.

## 8. Anti-Patterns
- **Hack Cascades**: Adding dummy return values or stubbing crashes without understanding the caller's contract.
- **Silent Stubs**: Catching exceptions or ignoring unmapped addresses to pretend execution progressed.
- **Premature Subsystem Rewrite**: Rewriting an entire subsystem in C++ because of a single failing indirect call.

## 9. Knowledge Capture
When a blocker is resolved:
- Document the investigation in `docs/investigations/` following the standard template.
- If a common gotcha was identified, record it in `docs/gotchas/`.

## 10. Completion Criteria
- Root cause is classified as `RECOMPILER`, `RUNTIME`, or `GAME-SPECIFIC`.
- Minimal fix is implemented and verified.
- Execution frontier successfully advances to the next verifiable boundary.
