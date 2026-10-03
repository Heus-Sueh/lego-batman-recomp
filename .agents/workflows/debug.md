---
description: Investigate a failure systematically.
---

# Debug

Investigate a failure systematically.

Do not begin by guessing fixes.

1. Reproduce the failure.

2. Capture:
   - error
   - logs
   - stack trace
   - inputs
   - environment
   - relevant state

3. Determine the last known-good stage.

4. Determine the first known-bad stage.

5. Reduce the search area.

6. Form explicit hypotheses.

For each hypothesis:

- state expected evidence
- perform the smallest useful experiment
- record the result
- reject or strengthen the hypothesis

7. Fix the demonstrated root cause.

8. Reproduce the original scenario.

9. Add regression validation when appropriate.

If the problem cannot be isolated, improve observability before making
additional speculative changes.