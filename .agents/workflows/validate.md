---
description: Determine whether an implementation actually satisfies its claims.
---

# Validate

Determine whether an implementation actually satisfies its claims.

1. Identify the claimed behavior.

2. Define observable success criteria.

3. Determine the strongest available validation method.

Prefer:

runtime behavior
→ automated tests
→ integration tests
→ traces
→ static inspection

4. Test success paths.

5. Test relevant failure paths.

6. Look for silent fallback behavior.

7. Look for stubs or partial implementations.

8. Classify the result:

VERIFIED
PARTIALLY_VERIFIED
FAILED
UNKNOWN

Explain what evidence supports the result.

Never classify an implementation as verified merely because it compiles.