# Investigation: [ID] - [Title]

## Revision
- **Date / source revision**: [date, commit and relevant working-tree changes]
- **Executable**: [selected input; do not assume a revision from its filename]
- **SHA-256**: [measure the actual input; never copy a previous report's hash]
- **Milestone / gate**: [use the canonical M0–M9 model in recomp-project]
- **Document status**: [active investigation / historical / superseded]
- **Evidence status**: [VERIFIED / PARTIALLY_VERIFIED / INFERRED / UNKNOWN]

## Execution Frontier
- **Last successful VA**: `0x00XXXXXX`
- **Current VA**: `0x00YYYYYY`
- **Failure**: [Description of failure, e.g., unknown indirect call, invalid memory access, missing import]
- **Target VA**: `0x00ZZZZZZ`

## Classification
- [x] UNKNOWN until evidence assigns ownership
- [ ] Recompiler (lifter instruction, CFG, jump table, calling convention, flags, x87)
- [ ] Runtime (guest memory, PE loading, Win32, dispatch, callback, threading, timing)
- [ ] Game-specific (middleware behavior, RenderWare driver, title-specific logic)

## Evidence
- [Trace logs, register dumps, disassembly snippet, stack trace]
- [Exact command, frame bound, input identity, expected and observed output]
- [Separate lifted behavior, host boundary behavior and declared stubs]
- [State what the experiment cannot establish; headless counters are not visual evidence]

## Hypothesis
- [Plausible explanation of what the target represents and why the failure occurred]

## Smallest Experiment
- [Targeted test or isolated run to test the hypothesis]

## Result
- [Observed result of the experiment]

## Root Cause
- [Confirmed mechanism of failure]

## Fix
- [Minimal correct implementation in the generic recompiler, runtime, or Win32 layer]

## Regression Test
- [Automated or reproducible check verifying the fix]
- [Negative control, result and confirmation that the mutation was reverted]
- [Highest validation level actually demonstrated, and remaining limitations]

## Upstream Candidate
- [ ] Yes: candidate for upstream `pcrecomp` contribution
- [ ] No: project-specific revision manifest or configuration

## Next Frontier
- **New Last Successful VA**: `0x00......`
- **Next Blocker / Target**: `0x00......`
- **Selection evidence**: [earliest demonstrated blocker, or justified independent work]

## Durable knowledge and supersession
- [Canonical documentation updated; preserve unique historical observations]
- [Rejected alternative, reason, and earlier guidance this result supersedes]
