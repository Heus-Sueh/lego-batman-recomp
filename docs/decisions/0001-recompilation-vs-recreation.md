# ADR 0001: Static Recompilation vs Source Recreation

## Context
When bringing up a legacy title like *Grand Theft Auto: San Andreas* on modern platforms, two fundamentally different architectural paths exist:
1. **Source Recreation / Behavioral Clone**: Re-implementing the game from scratch or reverse-engineering pseudocode by hand (e.g. creating synthetic C++ classes like `CGame`, `CWorld`, `CMenuManager` in `src/game/`).
2. **Static Recompilation**: Lifting the original compiled x86-32 instructions directly from `gta_sa.exe` into C/C++, compiling them natively, and wrapping them with a minimal host compatibility layer.

## Decision
We strictly choose **Static Recompilation**.

The original executable remains the authoritative reference for:
- Game initialization and main loop.
- Gameplay logic, missions, scripts, and state machines.
- AI, pedestrian logic, vehicle physics, and camera control.
- World simulation, streaming, save/load formats, and animations.
- RenderWare-side game logic.

Under NO circumstances shall any developer or AI agent rewrite, re-implement, or fake original game logic in hand-written C++ under `src/game/` or elsewhere. Any temporary stubs must be systematically replaced by recompiled x86-32 functions lifted from the game binary.

## Consequences
- **Positive**: Preserving original instructions makes original behavior the implementation reference. Behavioral equivalence still requires validation of lifting, ABI, runtime and timing; it is not guaranteed by this decision.
- **Negative**: Requires strict handling of 32-bit guest memory model, x86-32 ABI calling conventions, stack purge counts, and indirect calls (ICALLs).
- **Rejected Alternative**: Hand-written reimplementation was rejected because it inevitably leads to subtle divergences in physics, script execution, AI, and mission logic that are impossible to fully debug.
