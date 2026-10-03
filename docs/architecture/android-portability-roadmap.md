# Android ARM64 Static Recompilation Roadmap

> Review 2026-09-19: DEFERRED PROPOSAL; Android execution is UNKNOWN. Read [runtime architecture](../runtime-architecture.md) for current host allocation ownership. Do not replace working Windows allocation with POSIX-only code; backend selection belongs in pcmodern.

## Goal

Port the static recompilation runtime and the GTA: San Andreas recomp project to **Android ARM64** (`arm64-v8a`).

Target Architecture:
```text
                    lifted game code
                           │
                           ▼
                    portable runtime
                           │
              ┌────────────┼─────────────┐
              ▼            ▼             ▼
           Windows        Linux        Android
           x86_64         x86_64        ARM64
```

The lifted x86 code remains platform-independent. Platform differences are encapsulated strictly in the runtime and platform abstraction layers.

## Target Baseline
- ABI: `arm64-v8a`
- CPU: AArch64
- Toolchain: Android NDK (Clang + CMake + Ninja)
- Min SDK: Android API Level 26+

## Phased Execution

### Phase 0: Portability Baseline
- Define exact Android NDK toolchain and CMake toolchain file.
- Explicitly exclude 32-bit ARM (ARMv7). Target 64-bit ARM exclusively.

### Phase 1: Portable Image Loader
- Eliminate Windows-specific virtual memory APIs (`VirtualAlloc`, `MapViewOfFile`).
- Replace with generic POSIX `mmap` / `mprotect` memory mapping for guest image loading.

### Phase 2: CPU Semantics & Memory Model on ARM64
- Validate little-endian memory access and unaligned 32-bit load/store behavior on AArch64.
- Implement x87 80-bit extended precision float fallbacks (using double or soft-float if required).

### Phase 3: Android Platform Abstraction
- Windowing and event pump via Android NDK NativeActivity or SDL3 Android backend.
- Audio streaming via AAudio or Oboe (wrapped in SDL audio interface).
- Touch overlay controls mapped to DirectInput gamepad axes.

### Phase 4: Vulkan Renderer on Android
- Utilize Tier 4 Vulkan backend directly against Android Vulkan drivers.
- Implement mobile texture compression (ETC2, ASTC) or runtime transcode for DXT/BC surfaces.
