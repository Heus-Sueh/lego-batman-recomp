# libpcrecomp

Reusable guest execution bridge. **Used by the LEGO Batman executable today**, not
just a proposed extraction. This is maintained project code, distinct from the
upstream lifting toolbox in `thirdparty/pcrecomp/`.

## Responsibilities and consumers

| API / target | Current consumer |
|---|---|
| `PcRuntime`, `PcContext`, function registration and indirect dispatch | `src/runtime/bridge_adapter.cpp`, `src/runtime/dispatch.cpp` |
| Memory binding, checked translation and reads/writes | `src/runtime/guest_memory.cpp` and `.h` |
| PE module metadata | `src/runtime/section_loader.cpp` |
| Fault reporting | Generated unresolved-call bridge through `src/runtime/bridge_adapter.cpp` |
| Time/file import ABI bridges | Generated import dispatch via `recomp_call_time_import` / `recomp_call_fs_import` |

Public C headers are in `include/pcrecomp/`. `PcContext` contains integer registers
and flags, not a complete x86 CPU. `pcrecomp::bridge` is the CMake alias for the
static `pcrecomp_bridge` library (`src/runtime.cpp`). It has no SDL, game-code,
Win32 compatibility or host-service dependency.

Two additional static targets, `pcrecomp::win32_time` and `pcrecomp::win32_fs`,
are currently declared in the **root** CMakeLists.txt, using source files in this
directory. They translate guest arguments and stack cleanup into
[`win32compat`](../win32compat/README.md) calls. These adapters depend on both
`pcrecomp::bridge` and `win32compat::kernel32`; the bridge core does not.

## Use and validation

For the core in a parent CMake project:

```cmake
add_subdirectory(libpcrecomp)
target_link_libraries(your_target PRIVATE pcrecomp::bridge)
```

Create a runtime with `pc_runtime_create`, bind caller-owned storage with
`pc_bind_memory`, register functions/imports, and destroy the runtime after
execution stops. Consult the headers and `tests/runtime/foundation.cpp` for
argument, lifetime and error contracts. Guest addresses are 32-bit offsets,
never truncated host pointers.

From the repository root, `just test-runtime` builds/tests these libraries without
retail files, SDL or DXVK. `just build` links the same static libraries into the
game executable. `nm -C out/build/linux-amd64-release/legobatman` can confirm
linked symbols such as `pc_call_indirect` and `pc_bind_memory`; production entry
also exercises memory binding and dispatch through these APIs.

## Not migrated / not promised

The old generated register globals, x87/SSE state, generated direct lookup paths
and project coverage/frontier bookkeeping still exist outside this library.
File/time ABI adapters are not automatically built by this directory in isolation.
This is not an emulator, a game-logic implementation or a fully isolated SDK.
Use synthetic tests for generic fixes; keep title addresses out of this core.
