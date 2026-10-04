# D3D9 Guest-to-Host Boundary

Status: partial integration and design constraints, not verified rendering.
The current game frontier is recorded in the [README](../../README.md#project-status).

Intended flow: lifted graphics calls → guest COM wrapper/thunk → native D3D9
implementation → host graphics/window system. Application-side graphics
decisions remain lifted; external API translation must not assume a particular
engine, global device address or presentation call site.

## Required contracts

- Use validated guest offsets/handles, never truncated native pointers.
- Allocate wrappers outside mapped image data.
- Marshal method parameters, structures, buffers and output values.
- Match vtable slots and guest cleanup to API and binary evidence.
- Preserve reference counts, resource lifetime, errors and state.
- Verify the backend's actual window-system/version contract.
- Measure host presentation separately from guest simulation per
  [ADR 0002](../decisions/0002-decoupled-presentation-pacing.md).

Test marshalling with controlled resources, then exercise natural guest device
creation and rendering. Library loading, synthetic quads and Present counters
do not establish an authentic first frame. Comparative evidence is required.

See [dependency provenance](../../thirdparty/dxvk-native/README.md).
Later renderer replacement is not a prerequisite to investigating the earliest
demonstrated boot blocker.
