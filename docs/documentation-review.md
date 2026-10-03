# Documentation review — 2026-09-19

## Evidence and scope

VERIFIED: `just frontier` currently exits cleanly at one bounded presentation,
with 305 registered entries, 40 reached, 695 dispatches and zero reported
unresolved indirect calls. Last successful VA is `0x00727420`, current VA is
`0x00734e90`; these are trace values, not independently verified symbol names.
No current later blocker or interactive-gameplay gate is established by that run.

Source inspection: `scripts/analysis/lifter/analysis.py`, `dispatch_gen.py`,
`src/win32/win32_audio.cpp`, the runtime foundation and current tests.
The working tree already contains removal of `src/game/*` and `docs/future.md`;
this review does not finalize or expand those deletions.

## Inventory decisions

| Documents | Decision | Retained value / necessary qualification |
|---|---|---|
| Investigations 0001–0002 | Keep historical | Binary/ABI leads; fingerprints, labels and old milestone numbering require rechecking |
| 0003–0004 | Keep historical | Initial generation/loader/import integration; current ownership is runtime-architecture.md |
| 0005–0006 | Keep historical | Window, raster and Im2D experiments; counters alone do not prove visual/timing correctness |
| 0007–0009 | Keep as rejected approach | Handwritten menu/world/gameplay was removed; useful explanation of why ADR 0001 exists |
| 0010 | Keep historical, qualify claims | Geometry experiments are not proof of original world simulation/rendering |
| 0011 | Keep historical, qualify claims | Asset parser knowledge survives; complete streaming/gameplay claims are unsupported |
| 0012 | Keep historical | Removal of recreated logic and tooling history; bridge templates still contain synthetic behavior |
| 0013–0014 | Keep historical | Initialization and register investigations; copied hashes and previous target lists are not current evidence |
| 0015 | Keep historical with explicit supersession | ABI leads survive; pop-and-continue on unknown tails is superseded by diagnostic failure |
| investigations/TEMPLATE.md | Correct | Remove prefilled binary hash and obsolete gate examples; require provenance and evidence |
| runtime-architecture.md | Keep canonical | Current migrated contracts, tests and known limitations |
| runtime-architecture-audit.md | Keep baseline | Explains before/after ownership; not a competing live architecture |
| architecture/win32-compatibility.md | Update status and ownership | Distinguish implemented time path, legacy services and audio placeholders |
| architecture/renderware-subsystems.md | Keep conceptual reference | Signatures and lifecycle claims need binary/SDK evidence before implementation |
| architecture/dxvk-native-integration.md | Keep proposal, qualify | Loading DXVK does not establish full 3D parity or verified frame pacing |
| architecture/android-portability-roadmap.md | Keep deferred proposal | Not current scope; preserve portability considerations without speculative implementation |
| decisions/0001 and 0002 | Keep policies, correct guarantees | Decisions state intended behavior, not demonstrated equivalence or zero judder |
| gotchas/stack-balance-and-abi.md | Keep, clarify ABI inference | `ret N` proves cleanup bytes, not the calling convention by itself |
| gotchas/drm-crack-trampolines.md | Keep, restrict applicability | Revision-specific claims are not universal crack-section rules or safe patch recipes |
| gotchas/guest-simulation-pacing.md | Keep policy/context, qualify | 30 FPS is project policy; subsystem anecdotes and fidelity promises need comparative evidence |
| implementation/recurring-recipes.md | Correct active instructions | Use TOML and new runtime ownership, not the removed manual switch |
| implementation/stack-purge-discovery.md | Update active status | Metadata migration is done; first reachable ret and cache-key limitations remain |
| references/pcrecomp-upstream.md | Keep, update ownership | Codegen upstream versus reusable runtime versus title integration |
| workflow/bottleneck-audit.md | Keep historical measurements | Generator is modular now; cache is not a full binary-content hash |
| agent-audit.md | Keep historical migration audit | Old names and pending targets are not active inventory; skill-index is routing authority |
| skill-index.md | Keep routing | Fix machine-specific links; no implied milestone evidence |

No complete document was proven redundant: none is deleted in this review.
Keep historical URLs stable and qualify misleading claims at the top of each
report. Shared current contracts live in runtime-architecture.md; detailed
procedures remain in specialized skills rather than being copied here.

## Concrete corrections and open issues

- VERIFIED: generation lives in `scripts/analysis/lifter/`; `run_lift.py` is the
  entrypoint. Manual bridge mappings are emitted from TOML.
- VERIFIED: purge discovery returns the first reachable `ret`, without checking
  agreement across all exits. Its cache hashes file metadata and a subset of
  function addresses, not all binary/config contents. Exact ABI reliability
  remains PARTIALLY_VERIFIED; this review changes documentation, not discovery.
- VERIFIED: the legacy audio shim returns success/EOF placeholders; it does not
  substantiate a working DirectSound/audio pipeline.
- VERIFIED: generic time imports and runtime tests exist. Historical counts of
  18 or 23 tests describe earlier snapshots; use current test commands.
- UNKNOWN: behavioral equivalence, complete gameplay, visual correctness from
  headless logs, and the validity of copied historical binary hashes.

## Next implementation target

See [file API migration](implementation/file-api-migration.md). It is independent
runtime-foundation work already requested, not a claim that filesystem I/O is
the earliest demonstrated boot blocker. Preserve the observed frontier and
keep guest resource/streaming behavior in lifted code.
