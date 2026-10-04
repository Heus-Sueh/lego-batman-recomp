# Android ARM64 Static Recompilation Roadmap

> Review 2026-09-19: DEFERRED PROPOSAL; Android execution is UNKNOWN. Read [runtime architecture](../runtime-architecture.md) for current host allocation ownership. Do not replace working Windows allocation with POSIX-only code; backend selection belongs in pcmodern.

## Goal

Explore an **Android ARM64** (`arm64-v8a`) host for the reusable runtime and a
configured game. This is deferred design work, not a supported target.

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

Platform differences should be isolated in runtime/platform layers. Generated
code still requires portability validation on the actual compiler and CPU.

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
- Keep host memory APIs behind pcmodern, preserving existing Windows support.
- Add or validate an Android backend and verify guest address-layout constraints.

### Phase 2: CPU Semantics & Memory Model on ARM64
- Validate little-endian memory access and unaligned 32-bit load/store behavior on AArch64.
- Establish required x87 precision and exception behavior; double precision is
  not automatically equivalent to 80-bit arithmetic.

### Phase 3: Android Platform Abstraction
- Windowing and event pump via Android NDK NativeActivity or SDL3 Android backend.
- Audio streaming via AAudio or Oboe (wrapped in SDL audio interface).
- Touch overlay controls mapped to DirectInput gamepad axes.

### Phase 4: Vulkan Renderer on Android
- Utilize Tier 4 Vulkan backend directly against Android Vulkan drivers.
- Implement mobile texture compression (ETC2, ASTC) or runtime transcode for DXT/BC surfaces.
