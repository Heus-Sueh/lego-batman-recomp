# Selective win32hle integration

## Scope and provenance

Reviewed upstream pcrecomp commit `fc852e31765e8661cdde4326f2e2335090a05c30`
(2026-10-02):
[kernel32.c](https://github.com/sp00nznet/pcrecomp/blob/fc852e31765e8661cdde4326f2e2335090a05c30/runtime/win32hle/kernel32.c).
The upstream MIT notice is preserved in [win32hle-LICENSE.txt](win32hle-LICENSE.txt).
The toolbox submodule is not upgraded by this integration.

This is a selective semantic adaptation, not a drop-in copy: upstream uses
32-bit identity-addressed pointers; this project uses 32-bit guest offsets
inside a 64-bit host mapping. Host malloc pointers must not be truncated into EAX.
No upstream title-specific CRT behavior or handwritten game logic is imported.

## Boundary contract

`win32compat::GuestHeap` owns allocation metadata, not the backing mapping.
Generated stdcall bridges delegate GetProcessHeap, HeapCreate, HeapDestroy,
HeapAlloc, HeapFree, HeapSize and HeapReAlloc to this service. Stack cleanup
includes the return slot. The project binding borrows GuestMemory's mapping
and resets metadata before that mapping is released; shutdown requires guest
execution to have stopped.

- Guest arena: `[0x10000000, 0x20000000)`, preserving the former heap origin.
  Since the 2026-10-04 review, compatibility bump allocations occupy the separate
  range `[0x20000000, 0x30000000)`. The PE loader rejects images extending into
  runtime arenas (at or above `0x10000000`). This replaces the unsafe old bump
  origin `0x02000000`, which lay inside this revision's virtual .data section.
- Returned addresses are aligned to 16 bytes. Zero-length allocations occupy
  distinct minimum blocks. Freed gaps can be reused.
- Heap ownership is checked. HeapSize returns the requested size, or UINT32_MAX
  for an unknown block; the former fabricated 512-byte result is removed from
  generated bridges.
- HEAP_ZERO_MEMORY zeros allocated bytes and newly exposed realloc bytes.
  Failed realloc leaves the old allocation intact. IN_PLACE_ONLY fails if the
  request exceeds existing capacity; there is no adjacent-block expansion yet.
- Allocation flags supported: NO_SERIALIZE and ZERO_MEMORY, plus IN_PLACE_ONLY
  for realloc. Metadata operations remain serialized. Unsupported allocation
  flags fail instead of pretending exception-generation support.
- HeapCreate validates initial/max sizes but does not precommit initial space;
  maximum is an aligned allocation budget, not a full Windows heap model.

Not included: SEH/HEAP_GENERATE_EXCEPTIONS, exact Windows commit/page accounting,
LastError integration, arbitrary alternate-image arena placement, TLS, threads,
VirtualAlloc migration, or replacing native-signature legacy heap placeholders
in `src/win32/win32_memory.cpp`. Those placeholders are not used by the generated
heap bridges and must not be treated as equivalent entry points.

## Evidence (2026-10-03)

Subsystem: reusable Win32 memory boundary (M3 import integration), supporting
the current game-initialization frontier; no milestone promoted.

- VERIFIED: `just test-runtime`: 5/5 standalone tests, including allocation
  bounds, zeroing, reuse, heap ownership, exhaustion and realloc failure/data
  preservation. No game binary required.
- VERIFIED: `test_heap_bridges.py` compiles freshly generated bridge functions
  with a synthetic arena and executes all seven bridges, checking guest offsets,
  return values and stdcall ESP cleanup.
- VERIFIED: `just lift` and Linux release `just build` succeed.
- PARTIALLY_VERIFIED: bounded real entry still reaches 226 of 298 registered
  functions over 2909 dispatches, then stops at caller `0x004B01D0`, unresolved
  target `0x006D5510`, matching the pre-change frontier. Ownership remains UNKNOWN.
- `just test`: 22 tests, 12 failures, including stale frontier expectations and
  tests requiring boot beyond the unresolved call. Outside the sandbox the
  video initialization error disappears, but the 12 failures remain. The full
  suite is not green; behavioral equivalence and complete initialization are
  not established. Windows build/runtime validation was not performed.

The next boot investigation is the unresolved target, not speculative import
replacement. Other win32hle modules require their own ABI and behavior review.
