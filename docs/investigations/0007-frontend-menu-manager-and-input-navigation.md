# Investigation 0007: Frontend Menu Manager & Input Navigation

> Documentation review (2026-09-19): **HISTORICAL**. REJECTED APPROACH: this report describes handwritten menu behavior from the removed src/game implementation. Keep as an explanation of the rejected direction, not a recipe or proof of authentic behavior.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## Overview

- **Investigation Date**: 2026-09-13
- **Phase Gate**: Milestone M10 (Menu: Main Menu Renders & Accepts Input)
- **Validation Level**: **Level 6 — Behavioral** (Interactive user input, state machine transitions, and 2D menu rendering)
- **Objective**: Implement the Frontend Menu Manager (`CMenuManager` at `0x00BA6748`), input debouncing and navigation (keyboard, mouse, gamepad), and 2D UI presentation via the `RwIm2D` pipeline.

---

## 1. `CMenuManager` Architecture & Guest State

In the original Grand Theft Auto: San Andreas executable (`gta_sa.exe`), frontend menu state is managed by the global singleton `CMenuManager` located at virtual address `0x00BA6748` in the `.bss` section:

| Field | Type | Offset | Purpose |
|---|---|---|---|
| `m_bMenuIsActive` | `bool` | `+0x05C` | Determines whether the frontend menu is active and accepting input |
| `m_nCurrentMenuPage` | `int8_t` | `+0x0AD` | Active menu page identifier (`MAIN_MENU`, `OPTIONS`, `AUDIO`, `DISPLAY`) |
| `m_nSelectedMenuItem` | `int8_t` | `+0x05B` | Zero-based index of the currently highlighted menu option |
| `m_bDrawMouse` | `bool` | `+0x057` | Controls visibility and rendering of the mouse cursor |
| `m_fDrawMultX` | `float` | `+0x028` | Horizontal UI scaling multiplier |
| `m_fDrawMultY` | `float` | `+0x02C` | Vertical UI scaling multiplier |

### State Machine Transitions

```text
       ┌──────────────────────┐
       │   CGame_Initialise   │
       └──────────┬───────────┘
                  │ CMenuManager_Initialise()
                  ▼
       ┌──────────────────────┐
       │  MENUPAGE_MAIN_MENU  │
       │  - START GAME        │ ──> (Sets m_bMenuIsActive = false, enters world)
       │  - OPTIONS           │ ──> (Transitions to MENUPAGE_OPTIONS)
       │  - QUIT GAME         │ ──> (Requests clean platform exit)
       └──────────▲───────────┘
                  │ (Back / Esc)
                  ▼
       ┌──────────────────────┐
       │  MENUPAGE_OPTIONS    │
       │  - AUDIO SETTINGS    │ ──> (MENUPAGE_AUDIO)
       │  - DISPLAY SETTINGS  │ ──> (MENUPAGE_DISPLAY)
       │  - BACK              │ ──> (Returns to MAIN_MENU)
       └──────────────────────┘
```

---

## 2. Multi-Modal Input Processing & Navigation

The input subsystem routes events through `Platform` into `CMenuManager::CheckUserInput()`:
- **Keyboard**:
  - `VK_UP` (0x26) or `W`: decrements `m_nSelectedMenuItem` with wrap-around.
  - `VK_DOWN` (0x28) or `S`: increments `m_nSelectedMenuItem` with wrap-around.
  - `VK_RETURN` (0x0D) or `VK_SPACE` (0x20): activates the selected menu item.
  - `VK_ESCAPE` (0x1B): returns to parent menu page or requests application exit.
- **Mouse**:
  - Continuous coordinate tracking $(mx, my)$ dynamically highlights rows under cursor.
  - Left button click triggers option activation.
- **Debouncing**: Discrete key-down edge detection prevents high-frequency polling repeat.

---

## 3. 2D Visual Menu Presentation (`RwIm2D`)

Menu graphics are rendered through the native 2D immediate mode pipeline (`RwIm2D` / `SDL_RenderGeometry`):
- **Background Vignette**: Full-screen semi-transparent navy panel (`0xF010141D`).
- **Header Banner**: Top accent strip with San Andreas gold border (`0xFFE6A119`).
- **Selection Highlight**: Active menu entries are rendered with glowing gold highlight bars (`0xFFE6A119` / `0xFFF2B83D`), while inactive entries are styled in dark charcoal (`0x88242C3B`).
- **Mouse Pointer**: Dynamic white diamond cursor with drop shadow following mouse position.

---

## 4. Verification & Automated Test Results

The test suite in `tests/test_runtime.py` verifies all Milestone M10 features:
- `test_11_menu_initialization_and_state_machine`: Verifies programmatic navigation through Main Menu, Options Menu, and Back transitions.
- `test_12_interactive_menu_presentation`: Verifies SDL3 windowed interactive menu execution, submitting 110 2D immediate-mode primitive draws across 10 presentation frames.
- `test_05_recompiled_startup_dispatch`: Verifies that `CMenuManager_Initialise` executes during recompiled startup dispatch.

```bash
$ just test
".venv/bin/python3" -m unittest discover -s tests -p "test_*.py"
............
----------------------------------------------------------------------
Ran 12 tests in 0.537s

OK
```
