# Guest Simulation and Presentation Timing Pitfalls

Establish the selected revision's timers, update cadence and limiter behavior
through binary analysis and original-executable measurements. Do not infer
fixed-step, variable-step or a specific frequency from another game.
The manifest default is not original-binary evidence.

Physics, animation, scripts, input and audio can be timing-sensitive; their
actual dependencies require investigation rather than borrowed assumptions.

- Do not accelerate simulation merely to raise presentation FPS.
- Do not inject new timestep arithmetic without evidence.
- Headless frame/message-pump counters are not elapsed simulation time.
- Window creation and Present calls do not prove authentic rendered frames.
- Measure actual frame intervals, queue depth and latency, not only sleep calls.

Record revision, host conditions and observation scope. Keep guest progression
and host presentation measurements separate. See
[ADR 0002](../decisions/0002-decoupled-presentation-pacing.md).
