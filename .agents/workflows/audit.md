---
description: Audit the current implementation against its intended behavior.
---

# Audit

Audit the current implementation against its intended behavior.

Do not assume documentation accurately reflects implementation.

1. Determine intended behavior.

2. Inspect the actual implementation.

3. Inspect tests.

4. Inspect runtime validation where available.

5. Compare claims against evidence.

For every significant capability classify it as:

VERIFIED
PARTIALLY_VERIFIED
INFERRED
UNKNOWN
MISSING

Identify:

- incomplete implementations
- hidden stubs
- TODO-dependent behavior
- dead paths
- duplicated logic
- missing validation
- outdated documentation
- unsupported assumptions

Produce actionable findings ordered by dependency and impact.

Do not modify code unless explicitly requested.