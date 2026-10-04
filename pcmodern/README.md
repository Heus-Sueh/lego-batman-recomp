# pcmodern

Host services shared by runtime and compatibility code. **Used by the game build**
through direct memory/VFS calls and transitively through `win32compat`.

| Service | API | Current consumer |
|---|---|---|
| Anonymous host storage | `allocate_memory`, `release_memory` | `src/runtime/guest_memory.cpp` allocates/releases the guest mapping |
| Monotonic time, sleep and diagnostics | `time_ns`, `sleep_ms`, `log`, `HostCallbacks` | `win32compat/src/time.cpp`, runtime fault reporting |
| Case-insensitive guest path resolution | `VirtualFileSystem` | `src/platform/vfs.h` aliases this class; the adapter sets its root |
| Host file operations | `file_open/read/write/seek/tell/close` | `win32compat/src/fs.cpp` |

Headers live in `include/pcmodern/`. The CMake target is `pcmodern::core`, an alias
for static library `pcmodern_core`. It requires C++17 and `Threads::Threads`, but
has no dependency on libpcrecomp, win32compat, SDL, DXVK or retail files.

## Build and use

```cmake
add_subdirectory(pcmodern)
target_link_libraries(your_target PRIVATE pcmodern::core)
```

From the repository root, run `just test-runtime` for clock, file and memory
integration tests using synthetic inputs. Run `just build` for the game consumer.
To inspect linkage, search `nm -C out/build/linux-amd64-release/legobatman` for
`pcmodern::allocate_memory` or `pcmodern::time_ns`. Static libraries do not appear
as separate shared-library entries in `ldd`.

Guest address placement belongs to the caller: allocating storage here does not
reserve PE sections or establish a Win32 heap. Configure host callbacks before
execution; their context must outlive their installation. VFS roots/caches and
host callbacks currently use process-global state, so independent concurrent
guest runtimes are not an established capability. Path resolution is not a
security sandbox.

Windowing, audio, input and presentation remain in `src/platform/`, `src/win32/`
and `src/renderer/`; this library does not yet encapsulate the whole platform.
Linux is the exercised host path. Windows-specific implementation branches do
not imply that the full Windows game integration has been validated.
