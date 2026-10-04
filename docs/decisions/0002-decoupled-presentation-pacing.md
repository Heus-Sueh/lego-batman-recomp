# ADR 0002: Decoupled Simulation and Host Presentation Pacing

## Context

Guest simulation and host display refresh need not have the same frequency.
Changing the guest loop just to produce more frames can change behavior.
The manifest's current 30-tick default is configuration, not proof of the
selected binary's original timing model.

## Decision

Preserve verified guest timing semantics. Where supported, schedule host
presentation separately using monotonic clocks and completed guest frames.
Do not uncap simulation or inject new delta-time values to match monitor refresh.
Derive timer adaptations from binary evidence and measurements.

## Consequences

Separate scheduling requires explicit queue ownership, synchronization and
latency measurements; it does not by itself guarantee smoother output.
Any presentation interpolation must not mutate guest simulation.
Validate guest progression and host frame intervals separately.

This policy does not claim decoupling or behavioral equivalence has already
been demonstrated. See [timing pitfalls](../gotchas/guest-simulation-pacing.md).
