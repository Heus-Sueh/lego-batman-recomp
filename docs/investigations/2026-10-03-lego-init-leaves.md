# LEGO Batman: two early initialization leaves

## Revision and scope

- Date: 2026-10-03; source baseline `e6a9123` plus the existing dirty working tree.
- Input: configured `game/LEGOBatman.exe`, 7557120 bytes.
- Measured SHA-256: `981383d8c842a919dfecf61caf808a27745c3af39f4fa6197e3da88f5e6c842f`.
- Subsystem: early application initialization, M6 investigation; no new gate certified.
- Spec: N/A, bounded missing-function selection in revision metadata.
- Status: historical observation; obtain live status with `just frontier`.

## Evidence and fix

VERIFIED: baseline execution stopped at caller `0x004B01D0`, target `0x006D5510`,
with 226/298 functions reached and 2909 dispatches. The target is a 24-byte leaf
inside executable .text, missing from functions.toml. It dereferences the two
stack arguments, stores their values at `0x00B028E0` and `0x00B028E4`, then returns
at `0x006D5527` without argument cleanup. There are no callees or indirect branches.
Caller `0x004B01E1` passes two addresses, with combined/deferred stack cleanup at
`0x004B0284`. The descriptive symbol is locally assigned, not an original symbol.

After lifting that function, the next demonstrated target was `0x005C8110`.
VERIFIED: this 21-byte function calls already-lifted `0x0073CB50` with destination
`0x00AB29E8`, zero fill and length `0xA790`. It removes 12 argument bytes and ends
with plain ret at `0x005C8124`. Its caller at `0x004B0232` passes no arguments.
No competing handwritten implementation was found for either new address.

Classification: RECOMPILER pipeline coverage (missing revision function selection),
not a demonstrated instruction-lifting or heap-runtime defect. The fix is two
metadata entries, regenerated through `just lift`; no hand-edited generated code,
patches, or replacement game logic. Calling-convention labels remain
PARTIALLY_VERIFIED because a second independent call site was not established.

## Validation

- PASSED: `just lift`, Linux release `just build` (existing unused-function warnings).
- PASSED: `just test-runtime`, 5/5 tests.
- PASSED: production-path debugger probe:

  ```sh
  gdb -q -batch -x tests/probes/lego_init_frontier.gdb --args out/build/linux-amd64-release/legobatman
  ```

  The natural entry reaches both functions. The probe checks both stored values,
  all `0xA790` bytes after the clear function, and ESP delta +4 for each return.
  It does not modify guest inputs. The region may already contain zeros; this
  proves the postcondition, not a nonzero-to-zero transition or comparative behavior.
- PASSED negative control: passing `-ex 'set $expected_cleanup = 8'` before `-x`
  produces `FAIL: pair store or stack cleanup` and exit 1. The ordinary probe
  retains the correct +4 expectation. No production mutation is made.
- FAILED: `just test`, 22 tests, 12 failures. Stale frontier/boot-completion
  expectations and unresolved execution remain; the sandbox also lacks video.
  These assertions were not weakened. Windows validation was not performed.

Highest demonstrated ladder level: Level 3 (natural entry), with targeted
initialization memory/ABI checks; full Level 4 initialization remains unproven.
Both functions are RUNTIME_VERIFIED only for the observed natural-entry path.

## Next frontier

VERIFIED: 228/300 functions reached, 2912 dispatches. Last successful VA
`0x005C8110`, caller `0x004B01D0`, unresolved target `0x00401587` (UNKNOWN).
Inspection shows a five-byte jump at `0x00401587` to `0x00408900`; inspect that
destination and its ABI before selecting the thunk. The current inspect heuristic
reports zero size and prints adjacent thunks, so its inferred bounds must not be
used blindly. No renderer or gameplay progress is claimed.

Upstream candidate: no; these entries describe this executable revision.
