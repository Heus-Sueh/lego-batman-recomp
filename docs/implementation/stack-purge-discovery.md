# Automated Stack Purge Discovery

## Status

`PARTIALLY_VERIFIED`

The lifter now scans direct `call` and tail `jmp` targets in the active PE
text section, follows reachable direct control-flow branches, and records the
first reachable `ret` or `ret N`. The result is emitted as a generated
`bridge_ret_purgeN` fallback.

## Evidence

Historical snapshot (not the current inventory): the command below reported 1,675
direct targets with a reachable return cleanup value:

```text
.venv/bin/python3 scripts/analysis/run_lift.py \
  --revision config/revisions/gta_sa_compact \
  --output generated/recomp
[*] Loaded 145 function definitions.
[*] Discovered 1675 direct callees with proven stack purge values.
```

The generated code compiled successfully with Ninja. A bounded execution also
completed cleanly:

```text
gta_sa --headless --frames 300
Swapchain frame presented (300)
[Runtime] Bounded frame limit reached; leaving guest loop cleanly.
[Exit] Recomp runtime completed cleanly.
EXIT:0
```

## Configuration

Function metadata accepts `bridge`, `lift`, and `stack_purge`. Explicit bridge
metadata generates `recomp_lookup_manual` in
`scripts/analysis/lifter/dispatch_gen.py`; the old hardcoded manual switch has
been removed. Existing semantic bridges remain authoritative. Automatically discovered
purge bridges are fallback entries only.

Example:

```toml
[[function]]
rva = 0x00468D50
name = "CTheScripts_Init"
convention = "cdecl"
stack_purge = 0
lift = false
bridge = "bridge_func_00468D50"
```

## Open work

- Completed: legacy manual switch aliases migrated to TOML. Legacy bridge
  implementations still need behavioral review; metadata migration is not
  proof that those implementations are correct.
- Open: check all reachable exits for conflicting purge values. The current
  algorithm returns the first reachable return; it does not prove every path.
- Open: strengthen cache invalidation. `analysis.py` currently hashes mtime,
  file size, function count and boundary subsets of function addresses; it does
  not hash the full executable, complete metadata or analysis implementation.
- Add a function-prologue heuristic and call-graph filtering before promoting
  all automatically discovered candidates to runtime dispatch.
- Add a regression test that compares discovered purge values with known
  `ret N` fixtures.
