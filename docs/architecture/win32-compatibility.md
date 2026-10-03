# Win32 Compatibility Boundary in GTA: San Andreas

## Architecture Overview

Reviewed 2026-09-19. The canonical ownership and migration status are in
[runtime architecture](../runtime-architecture.md). The migrated time imports
cross `libpcrecomp -> win32compat -> pcmodern`; other APIs still use the legacy
integration described below. This inventory does not imply full API coverage.

The recompiled game logic executes in an x86-32 guest memory model and expects standard 32-bit Win32 and DirectX APIs. The compatibility layer in `src/win32/` translates these calls to project-owned platform abstractions (`src/platform/`), which are backed by SDL on host operating systems (Linux and Windows).

```text
Recompiled Game Logic (gta_sa)
             │
             ├── kernel32 / user32 / winmm
             ├── DirectInput 8
             ├── DirectSound
             └── Direct3D 9
                    │
                    ▼
     Compatibility Layer (src/win32/)
                    │
          ┌─────────┼─────────┐
          ▼         ▼         ▼
         SDL      POSIX    Renderer
     (Host OS)  (System)  (Pipeline)
```

## Subsystem Translations

### 1. Window Management & Event Pump (`user32.dll`)
- **APIs**: `RegisterClassA`, `CreateWindowExA`, `ShowWindow`, `UpdateWindow`, `PeekMessageA`, `GetMessageA`, `TranslateMessage`, `DispatchMessageA`.
- **Mapping**:
  - `CreateWindowExA` registers the main host window through `SDL_CreateWindow`.
  - The Win32 message pump translates `SDL_PollEvent` events into Windows message structures (`WM_KEYDOWN`, `WM_KEYUP`, `WM_MOUSEMOVE`, `WM_LBUTTONDOWN`, `WM_ACTIVATE`, `WM_CLOSE`).
  - Virtual `HWND` handles map to internal window descriptors via a 32-bit `HandleTable`.

### 2. DirectInput 8 (`dinput8.dll`)
- **Evidence**: PARTIALLY_VERIFIED integration; complete device semantics and
  preservation of original deadzones/sensitivity require comparative tests.
- **APIs**: `DirectInput8Create`, `IDirectInput8A::CreateDevice`, `IDirectInputDevice8A::SetDataFormat`, `IDirectInputDevice8A::Acquire`, `IDirectInputDevice8A::GetDeviceState`.
- **Mapping**:
  - Provides a lightweight C++ COM shim implementing `IDirectInput8A` and `IDirectInputDevice8A`.
  - Keyboard device queries SDL key states.
  - Mouse device queries `SDL_GetRelativeMouseState`.
  - Host gamepad axis/button mapping must preserve guest processing; original deadzones and sensitivity have not been established by this document.

### 3. DirectSound (`dsound.dll`)
- **Status**: PROPOSED boundary, not a verified implemented audio pipeline.
  The inspected `src/win32/win32_audio.cpp` contains Vorbis success/EOF stubs.
  They are not evidence of playback, decoding or DirectSound buffer support.
- **APIs**: `DirectSoundCreate8`, `IDirectSound8::CreateSoundBuffer`, `IDirectSoundBuffer::Lock`, `IDirectSoundBuffer::Unlock`, `IDirectSoundBuffer::Play`.
- **Mapping**:
  - Future host audio backends may use SDL audio streams behind runtime interfaces.
  - Circular-buffer semantics and audio playback require separate implementation and tests.

### 4. Direct3D 9 (`d3d9.dll`)
- **APIs**: `Direct3DCreate9`, `IDirect3D9::CreateDevice`, `IDirect3DDevice9::Present`, `IDirect3DDevice9::Reset`.
- **Mapping**:
  - **Tier 0**: System D3D9 passthrough or translation layer (e.g. DXVK).
  - **Tier 1**: Intercepts `IDirect3DDevice9::Present` or RenderWare presentation calls to route backbuffer image into the host SDL/Vulkan swapchain with monotonic pacing.

### 5. File I/O & Case-Insensitive VFS (`kernel32.dll`)
- **APIs**: `CreateFileA`, `ReadFile`, `WriteFile`, `SetFilePointer`, `CloseHandle`, `FindFirstFileA`, `FindNextFileA`.
- **Mapping**:
  - `pcmodern::VirtualFileSystem` resolves separators/case through the old `src/platform/vfs.h` alias. File I/O and handle semantics still live in `src/win32/win32_fs.cpp`; the [next migration slice](../implementation/file-api-migration.md) addresses that boundary.
  - Handles map to 32-bit descriptors in `HandleTable` to avoid 64-bit host pointer truncation bugs.

### 6. Time and sleep (migrated)

VERIFIED by `just test-runtime`: GetTickCount, timeGetTime and QPC/QPF use the
shared pcmodern clock through win32compat. The optional libpcrecomp time adapter
marshals guest stdcall arguments. Sleep reaches the replaceable host sleep
service. See the canonical runtime document for ABI and validation limitations.
