# Recurring Recompilation Workflows

Start with the target manifest, selected revision and `just frontier`.
Failures remain UNKNOWN until evidence establishes ownership.
Detailed methods live in the [project skills](../skill-index.md).

| Target evidence | Next action |
|---|---|
| Verified import slot | Check guest marshalling and external API semantics |
| Original application code | Verify bounds/ABI and lift through revision metadata |
| Thunk or tail jump | Follow actual control flow and verify the destination ABI |
| Bounded indirect branch | Recover cases and bounds from the selected binary |
| Inconsistent decoding | Recheck identity, code/data classification and protection |

Address ranges and decompiler names alone do not establish identity.
A short jump is not proof of DRM. Never copy another revision's patch formulas.

## Add a lifted function

1. Inspect `just inspect <address>`; verify bounds against original instructions.
   The heuristic may overrun thunks or return invalid sizes.
2. Check reachable exits, callers, cleanup, direct callees and indirect targets.
3. Update the selected revision's `functions.toml` with address, size, descriptive
   name, convention, purge, `lift = true` and scoped evidence/confidence.
   The current schema's `rva` field holds guest VAs; follow the existing schema.
4. Run `just lift`, `just build`, relevant tests and `just frontier`.
5. Check a meaningful stack/register/memory effect and record the next blocker.

Never hand-edit generated code or replace missing application functions with
success stubs. Library-looking code is not automatically an external boundary.

## Boundary and control-flow fixes

Use libpcrecomp for ABI marshalling, win32compat for Windows semantics and
pcmodern for host services where supported. Other APIs remain in project shims.
Check pointer widths, handles, buffers, errors and return-slot/argument cleanup.
Add synthetic regression tests and negative controls.

Prefer metadata or a reusable lifter fix for unusual control flow.
Deterministic patches are a last resort, requiring preimage checks, revision
identity and validation. Do not discard a wrapper's side effects or interpret
protected bytes as ordinary instructions.

See [runtime ownership](../runtime-architecture.md).
