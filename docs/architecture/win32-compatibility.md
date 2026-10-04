# Win32 Compatibility Boundary

External Windows APIs translate guest arguments/resources into host services.
Original application logic remains lifted. Extraction is partial.

| Area | Current path | Limit |
|---|---|---|
| Time | libpcrecomp adapter → win32compat → pcmodern | Synthetic ABI/clock checks, not full game timing |
| Sleep | Project shim → win32compat → pcmodern | Host sleep service |
| Files | ABI adapter/project wrappers → win32compat → pcmodern | Tested lifecycle subset, not all sharing/security semantics |
| Heap | Generated bridge → project binding → GuestHeap | Not complete Windows heap/SEH behavior |
| Window/input/audio | Project Win32/SDL integration | Incomplete coverage; placeholders are not working devices |
| D3D9 | Guest COM boundary → native backend | Library loading is not a verified rendered frame |
| Other threading/memory APIs | Project shims | Must be audited individually |

Never truncate host pointers into guest handles. Verify widths, buffer layout,
bounds, ownership, error results and return-slot plus argument cleanup.
Input must reach guest callbacks/device APIs rather than guessed state globals.
Audio must preserve buffer semantics rather than invent content or success.

See [runtime architecture](../runtime-architecture.md),
[file migration](../implementation/file-api-migration.md),
[heap integration](../references/win32hle-integration.md) and
[graphics boundary](dxvk-native-integration.md).
