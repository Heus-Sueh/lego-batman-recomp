# ADR 0001: Static Recompilation vs Source Recreation

## Context

Recreating application behavior and lifting its original instructions are
different approaches. This repository chooses static recompilation of the
executable selected by `config/target.toml`.

## Decision

Original initialization, gameplay, scripts, simulation, streaming, saves,
animation and application-side rendering logic must execute through lifted
x86-32 instructions. The matching original executable is authoritative.

Do not replace original behavior with handwritten C++, synthetic state changes
or success-returning placeholders. Missing guest functions remain visible
blockers until their authentic behavior is lifted.

External boundaries may be replaced when necessary: operating-system services,
files, graphics APIs, input, audio and synchronization. Preserve their guest
contracts and keep implementations in the proper runtime/platform layer.

## Consequences

Fix generated-code defects in metadata or the lifter and regenerate.
Verify binary identity, ABI, memory effects and timing for the selected revision.
Preserving instructions does not guarantee equivalence: independent validation
of runtime services and translated instruction semantics is still required.
Source recreation is outside this project's scope.

See [contribution workflow](../../CONTRIBUTING.md) and
[validation policy](../../.agents/skills/recomp-validation/SKILL.md).
