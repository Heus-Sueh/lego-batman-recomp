# Publication review and runtime library usage

Scope: pending changes over `e6a9123`, reviewed 2026-10-04. This is an engineering
review of build ownership, changed code, tracked-file hygiene and validation;
it is not a certification of every instruction or third-party binary.

## Corrected findings

- VERIFIED: compatibility allocation previously began at `0x02000000`, inside
  the selected image's virtual .data range (`0x00933000` through `0x02A15490`,
  exclusive). Zeroing allocations could overwrite original data. No particular
  observed frontier failure is attributed to this without further evidence.
- Fixed: compatibility allocations now use `[0x20000000, 0x30000000)`, separate
  from the Win32 heap `[0x10000000, 0x20000000)`. Shutdown resets the cursor.
  Requests are bounded before alignment; zero and oversized requests fail.
  PE images extending beyond `0x10000000` are rejected before mapping, rather
  than claiming arbitrary-image support. Short optional headers are rejected.
- Added `guest_memory_arenas` synthetic regression: original-data sentinel,
  alignment, rejected sizes, separated heaps and shutdown/reinitialization.
  Passing an extra argument to the executable enables a deliberately incorrect
  old-origin expectation as a negative control.
- Removed the stale GTA-address write diagnostic from generic guest memory.
- Corrected active documentation about the removed legacy adapter; preserved
  historical descriptions as explicitly historical. Removed the machine-specific
  executable path from checked-in PE analysis metadata.

## Library evidence

VERIFIED: the root build links `pcrecomp::win32_time` and `pcrecomp::win32_fs`,
which transitively link `pcrecomp::bridge`, `win32compat::kernel32` and
`pcmodern::core`. `nm -C` on the game executable confirms linked implementations
of `pc_bind_memory`, `pc_call_indirect`, `pcmodern::allocate_memory`,
`pcmodern::time_ns`, `win32compat::get_tick_count` and `GuestHeap::alloc`.
Source call chains show active memory, dispatch, file/time and heap consumers.
Each library README lists those consumers and incomplete migration boundaries.
This proves use, not full Win32 fidelity or that every exported API is reached.

## Distribution hygiene

The existing three DXVK shared-library/symlink entries are removed from the index
but retained locally. Exact build provenance was not recorded; no redistribution
claim is made for those artifacts. Ignore rules now cover versioned `.so.*`.
Upstream licensing/reference information is recorded beside the retained headers.
Previously published history is not rewritten. Game files, generated code and
build outputs are not included in the new commit.

The approved removal of the legacy adapter, RW/asset sources and historical GTA
documents is included. Those files remain recoverable from the preceding commit.

## Validation and limits

- Linux game build: PASSED (existing generated unused-function warnings).
- Runtime suite: 6/6 PASSED; library dependency checks remain enforced.
- Python suite: 10/22 PASSED, 12 FAILED, unchanged in count. Stale boot/frontier
  expectations and unresolved guest code remain; sandbox video is unavailable.
- Natural frontier remains caller `0x004B01D0`, target `0x00401587`, last successful
  `0x005C8110`. No milestone promotion; natural entry plus targeted memory checks
  do not prove renderer/gameplay behavior.
- Local Markdown links and `git diff --check`: PASSED.
- Windows game build and comparative behavior: NOT VERIFIED.

Known limitations are retained rather than hidden by weakening assertions.
Next execution work remains inspection of the thunk target `0x00408900`.
