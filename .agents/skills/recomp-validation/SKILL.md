---
name: recomp-validation
description: "Enforce evidence rules, evaluate claims against the 7-level validation ladder, execute negative controls, maintain regression tests, and conduct comparative behavioral verification for static recompilation."
---

# recomp-validation: Validation Ladder & Evidence Policy

## 1. Purpose
Provide objective, reproducible verification of static recompilation progress. Classify claims according to the 7-level validation ladder, enforce rigorous evidence policies (`VERIFIED` vs `INFERRED`), execute negative controls, and maintain regression tests.

## 2. When to Use
- When claiming a milestone, feature, or bug fix is complete.
- When verifying behavioral equivalence between recompiled code and the original binary.
- When designing automated regression tests for recompiler or runtime bugs.
- When validating that a diagnostic probe or test harness can detect actual defects (negative controls).
- Do NOT use for debugging active crashes (use `$recomp-debug`).
- Do NOT use for instruction disassembly (use `$binary-analysis`).

## 3. Inputs
- Test scenario or input script.
- Observable execution artifacts: execution traces, register dumps, log files, rendered frame captures, memory state dumps.
- Reference behavior observations from the original binary (if comparative testing).

## 4. Workflow

### Step 1: Claim Classification via the 7-Level Ladder
Every assertion of progress must state the exact validation level demonstrated:

```text
Level 1: Static       - Generated C/C++ source code is syntactically valid.
Level 2: Build        - Project compiles and links without errors.
Level 3: Entry        - Execution reaches mainCRTStartup / WinMain naturally.
Level 4: Init         - Subsystem initialization stages execute (window, memory, data).
Level 5: Visual       - Expected splash screen, menu, or 3D frame renders to display.
Level 6: Behavioral   - Input, physics, timing, audio, and save state operate authentically.
Level 7: Comparative  - Recomp and original binary produce bitwise or perceptually identical results for identical inputs.
```

> [!CAUTION]
> **Never claim a higher level from lower-level evidence.**
> - Code compiling and linking only proves **Level 2 (Build)**. It does NOT prove Level 3, 4, or 6.
> - A window opening only proves **Level 4 (Init)**. It does NOT prove gameplay works.

### Step 2: Evidence Tagging Policy
Every recorded finding, address label, and status record must be marked with an explicit confidence tag:

| Tag | Definition | Acceptance Rule |
|---|---|---|
| `VERIFIED` | Directly reproduced from the authoritative binary/artifact or demonstrated by execution traces, automated tests, or direct register/memory dumps. | Accepted as proven only for the stated claim and evidence dimension. |
| `PARTIALLY_VERIFIED` | Static binary evidence matches, but runtime path has not executed end-to-end. | Requires runtime confirmation before closing task. |
| `INFERRED` | Deduced from naming conventions, comments, decompiler output, or similarity to other games. | Must NEVER be presented as fact without explicit qualification. |
| `UNKNOWN` | Conflicting, insufficient, or unanalyzed evidence. | Open question requiring investigation. |

Tag the claim at the scope actually tested. A claim may be `VERIFIED` for binary
provenance and still be `PARTIALLY_VERIFIED` end-to-end. Record which evidence
dimensions were exercised:

1. **Binary provenance** — bytes, function bounds, ABI, and control flow match
   the authoritative executable.
2. **Generated ownership** — regenerated output comes from metadata/lifter code;
   no competing hand-written body or stale generated edit owns the same VA.
3. **Deterministic runtime probe** — controlled arguments produce the expected
   register, stack, memory, or call-trace transition.
4. **Host integration** — a real host event or API crosses every compatibility
   boundary used in production.
5. **User-visible behavior** — the natural application path produces the expected
   interaction or frame without forced flags or direct state injection.
6. **Comparative behavior** — the same input is compared with the original.

A probe that calls `MainWndProc` directly can verify its guest-side behavior,
but cannot by itself verify SDL event capture, the host message queue, or physical
keyboard interaction. Likewise, forced state transitions are diagnostics, not
proof that the natural transition works.

### Step 3: Negative Controls
Before relying on a validator, test, or assertion:
1. Introduce a deliberate small mutation in an isolated/temporary test path (e.g.,
   alter the probe input or expected value); do not contaminate a shared dirty tree.
2. Run the validator to confirm that it reports a clear failure.
3. Revert the mutation and verify the validator passes.
4. If a validator cannot fail when code is intentionally broken, it is useless.

### Step 4: Regression Test Creation
For every generic bug fix in the recompiler, runtime, or Win32 shims:
1. Create a minimal, isolated test that reproduces the boundary condition.
2. Ensure the test does NOT require proprietary retail game assets to run.
3. Integrate into the automated test suite (`tests/`).

### Step 5: Environment-Dependent Validation

- Separate implementation failures from unavailable infrastructure such as a
  missing display, GPU, audio device, game asset, or target toolchain.
- Report checks as `PASSED`, `FAILED`, or `BLOCKED_ENVIRONMENT`; never count an
  environment-blocked check as passed or silently remove it.
- Run the strongest asset-independent/headless probe available, then state the
  exact end-to-end evidence still missing.
- Do not weaken a windowed or physical-input assertion merely to make CI green.

## 5. Evidence Rules
- Documentation, comments, and decompiler output do NOT constitute proof of runtime correctness.
- Claims of equivalence require observable comparative logs (e.g. comparing state dumps between original executable under Wine/Windows and recompiled binary).

## 6. Validation
- Run test harness: `python3 -m unittest discover -s tests -p "test_*.py"`.
- Run headless execution validation with frame bounding: `out/build/<preset>/recomp --frames N --headless`.
- For ABI/callback work, assert stack restoration and at least one exact observable
  register or memory transition, not only process exit code.
- For input, rendering, and audio, add a real-device/windowed check when the active
  environment supports it and keep the result distinct from deterministic probes.

## 7. Failure Handling
- If a test fails: isolate whether the defect is in the test harness or in the recompiled binary before altering the code.
- If comparative verification diverges: identify the first diverging guest state (register, memory location, or floating-point precision).

## 8. Anti-Patterns
- **Weakening Assertions**: Relaxing test tolerances or disabling checks to make a broken build pass.
- **Silent Promotion**: Promoting an `INFERRED` address or convention to `VERIFIED` without runtime proof.
- **Process Start == Boot**: Declaring "the game boots" because the process launches, even if it crashes in early CRT initialization.

## 9. Knowledge Capture
When validation completes:
- Record validation level and evidence in the investigation journal (`docs/investigations/`).
- Update the milestone gate status in `docs/` and `AGENTS.md`.

## 10. Completion Criteria
- Demonstrated validation level (1–7) is explicitly declared.
- Evidence dimensions exercised and bypassed are explicitly stated.
- All conclusions carry evidence tags (`VERIFIED`, etc.).
- Automated regression test exists for generic fixes.
- Environment-blocked checks remain visible with a concrete follow-up.
