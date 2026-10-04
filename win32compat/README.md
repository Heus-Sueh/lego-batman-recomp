# win32compat

Reusable implementations of selected Windows service semantics. **Used by the
LEGO Batman integration**, via the libpcrecomp file/time ABI adapters and the
project guest-heap binding. This is not a complete Windows compatibility layer.

| Implemented area | Production path |
|---|---|
| Tick count, multimedia time and performance counter/frequency | Generated time imports → libpcrecomp adapters → `src/time.cpp` → pcmodern |
| Sleep | Generated Sleep bridge → project thread shim → `src/time.cpp` → pcmodern |
| File services and numeric handles | Generated file imports → libpcrecomp adapters → `src/fs.cpp` → pcmodern |
| Guest heap allocation, free, size, create/destroy and realloc | Generated heap bridges → `src/runtime/guest_heap.cpp` → `GuestHeap` |

Public headers are in `include/win32compat/`. The static target
`win32compat::kernel32` aliases `win32compat_kernel32` and depends on
[`pcmodern::core`](../pcmodern/README.md). It does not depend on libpcrecomp,
guest registers, SDL or a particular game. Guest stack decoding belongs to
adapters, not to this library.

## Build and validation

```cmake
add_subdirectory(win32compat)
target_link_libraries(your_target PRIVATE win32compat::kernel32)
```

The directory adds its sibling `pcmodern` if that target is not already present.
Run `just test-runtime` from the repository root for synthetic time, filesystem
and heap checks. `tests/test_heap_bridges.py` additionally compiles generated
bridges against a synthetic arena and checks stdcall cleanup. No game files are
required for these checks. `just build` links this library into the game;
`nm -C out/build/linux-amd64-release/legobatman` exposes `win32compat::GuestHeap`
and time/file symbols. Symbol presence proves linkage, not that every API has
been reached or is behaviorally equivalent to Windows.

## Limitations and ownership

`GuestHeap` borrows caller-owned mapped storage and returns guest offsets. The
caller must keep that mapping alive and stop execution before destroying it.
The supported heap flags and differences from Windows are documented in
[the selective win32hle integration](../docs/references/win32hle-integration.md).
Allocation failure, ownership and bounds checks are tested; SEH and exact Windows
page/commit behavior are not implemented.

File/time state has process-global components; this is not a per-runtime OS
instance. File services are a subset, not full Win32 sharing/security semantics.
Many APIs, including windowing, threads, DirectInput, audio and D3D9, still live
in `src/win32/`. Legacy native-signature heap placeholders there are **not** the
same entry points as the generated heap bridges. Do not claim those APIs are
fixed merely because this library's tests pass. Keep title-specific logic and
original game function bodies outside this library.
