# Stack Purge Discovery

Discovery in `scripts/analysis/lifter/analysis.py` provides candidate cleanup
sizes, not function implementations or proof of a complete calling convention.

The LEGO Batman driver disables automatic return-only guest bridges.
Unselected callees must remain visible unresolved calls; do not restore those
fallbacks to obtain a clean exit or higher coverage count.

Check all reachable exits and relevant callers. Plain ret and ret-with-immediate
establish cleanup behavior but do not alone distinguish every convention.
Tail calls inherit destination behavior.

The current heuristic can return the first reachable return and miss conflicting
exits. Its cache is not a complete content fingerprint. Verify binary identity
independently and recheck hints after analysis changes.

Use the selected revision and canonical `just lift` driver. Explicit bridges
require semantic and ABI justification beyond a `stack_purge` value.
Test discovery with synthetic return/tail/conflicting-exit fixtures and verify
stack restoration during actual execution.

See [ABI pitfalls](../gotchas/stack-balance-and-abi.md).
