---
name: platform-port
description: "Implement host platform integration using SDL, manage the Win32 compatibility boundary (kernel32, user32, winmm, DirectInput, DirectSound), prevent 64-bit pointer truncation, and maintain cross-platform support (Linux, Windows, Android)."
---

# platform-port: Host Platform Integration & Win32 Compatibility

## 1. Purpose
Provide host operating system integration and API compatibility shims for static recompilation. Encapsulate host services (windowing, events, input devices, audio streaming, threading, filesystem) behind clean platform abstractions, translate legacy Win32/DirectX API expectations, and ensure zero platform pollution in recompiled game logic.

## 2. When to Use
- When implementing or maintaining host platform abstractions (`src/platform/`).
- When implementing or fixing Win32 compatibility shims (`src/win32/`).
- When handling input mapping (keyboard, mouse, gamepad) or audio output.
- When resolving filesystem path differences (case sensitivity, path separators).
- When validating builds across target operating systems (Linux x86_64, Windows x86_64, Android ARM64).
- Do NOT use for presentation swapchain pacing or graphics migration (use `$native-renderer`).
- Do NOT use for binary instruction lifting (use `$static-recomp`).

## 3. Inputs
- Target host OS (Linux, Windows, Android) and toolchain (Clang, GCC, MSVC, Android NDK).
- Required Win32/DirectX API imports discovered during PE analysis or execution.
- Host backend dependencies (SDL, POSIX, C++23 standard library).

## 4. Workflow

### Step 1: Enforce Zero Platform Pollution
Recompiled game logic must remain completely independent of host operating systems.
- Never place `#ifdef _WIN32` or `#ifdef __linux__` inside recompiled game logic.
- Game code calls standard Win32/DirectX APIs or generic guest runtime functions.
- The compatibility layer (`src/win32/`) translates those calls to platform interfaces (`src/platform/`).
- The platform layer implements interfaces using SDL and standard C++23.

### Step 2: Safe Handle Translation (`HandleTable`)
In a 32-bit guest environment executing on a 64-bit host:
- Casting 64-bit host pointers (`void*`, `SDL_Window*`, file descriptors) directly into 32-bit `HANDLE`, `HWND`, or `HINSTANCE` types causes fatal high-word truncation.
- Always use a 32-bit `HandleTable` that assigns small integer indices (e.g. `1, 2, 3...`) to host resources:
  ```cpp
  uint32_t guest_handle = g_handle_table.Register(host_pointer);
  HostType* ptr = g_handle_table.Get<HostType>(guest_handle);
  ```

### Step 3: Win32 Message Pump & Input Translation
1. **Windowing & Events**:
   - `CreateWindowExA` creates the native window via `SDL_CreateWindow`.
   - `PeekMessageA` / `GetMessageA` poll events via `SDL_PollEvent` and post synthesized Win32 messages (`WM_KEYDOWN`, `WM_MOUSEMOVE`, `WM_ACTIVATE`).
   - Preserve the guest's 32-bit message contract: field widths/layout, queue
     ordering, filters, `PM_REMOVE`, key-up/previous-state bits, and callback ABI.
   - SDL updates host input state and enqueues boundary events. It must not write
     guessed game/menu/pad addresses or decide guest state transitions directly.
   - `DispatchMessageA` must reach the guest window procedure through production
     dispatch; an empty success stub is not message delivery.
2. **DirectInput 8 Shimming**:
   - Implement COM shims for `IDirectInput8A` and `IDirectInputDevice8A`.
   - Map `SDL_Gamepad` axes and buttons to standard DirectInput joystick state structures.
   - Maintain original deadzone and sensitivity curves.

### Step 4: DirectSound Audio Translation
1. Back DirectSound secondary buffers with SDL audio streams (`SDL_AudioStream`).
2. Support circular buffer lock/unlock semantics (`IDirectSoundBuffer::Lock`/`Unlock`) to allow the guest to stream audio data safely into host audio pipelines.

### Step 5: Case-Insensitive Virtual Filesystem (VFS)
1. Linux and Android filesystems are case-sensitive; legacy Windows games assume case-insensitivity.
2. Intercept Win32 file calls (`CreateFileA`, `FindFirstFileA`).
3. Normalize path separators (`\` $\rightarrow$ `/`) and perform case-folding lookups on disk before opening files.

## 5. Evidence Rules
- Validate Linux and Windows build paths when the corresponding toolchains are
  available. An unavailable target is an explicit validation gap, not permission
  to claim cross-platform verification and not an implementation failure.
- All handle lookups must include validity checks with clear diagnostic errors on invalid handles.
- Distinguish a deterministic boundary probe from real host integration. Directly
  invoking a guest callback proves the callback path, not SDL capture or a physical
  device event.

## 6. Validation
- Run unit tests for handle tables, VFS path lookups, and input mapping.
- For event work, verify queue filtering/removal and exact guest message layout,
  then assert callback stack restoration and an exact guest state transition.
- Verify native window creation, physical keyboard/gamepad response, and audio
  playback on an active host when those devices are available. Report unavailable
  display/audio/device checks as `BLOCKED_ENVIRONMENT` rather than weakening them.

## 7. Failure Handling
- If a crash occurs in a Win32 shim: check for 64-bit pointer truncation or stack purge mismatch in the shim signature.
- If a game cannot find data files: check VFS case-mapping logs for unresolved path lookups.

## 8. Anti-Patterns
- **Pointer Truncation**: Casting 64-bit host pointers directly to 32-bit guest integers without a handle table.
- **Scattered `#ifdef`s**: Polluting game logic with host OS macros.
- **Hardcoded Path Separators**: Relying on Windows backslashes in cross-platform filesystem calls.
- **Guest-State Injection**: Writing game-specific menu, pad, or world addresses
  from SDL/Win32 code instead of delivering the original boundary event.
- **Stubbed Delivery**: Returning success from message dispatch without invoking
  the registered guest callback/window procedure.

## 9. Knowledge Capture
When platform porting work completes:
- Document newly implemented API shims in `docs/architecture/win32-compatibility.md`.
- Record any host-specific quirks or build toolchain requirements in `docs/gotchas/`.

## 10. Completion Criteria
- Win32/DirectX API calls resolve to clean platform abstractions.
- The changed boundary has an exact contract test and a production-path runtime
  observation at the strongest level supported by the environment.
- Host windowing, input, and audio claims are limited to the devices actually tested.
- Build compiles without warnings on tested target host platforms; untested targets
  remain explicit validation gaps.
