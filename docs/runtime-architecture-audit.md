# Runtime Migration Risks

Maintained ownership checklist, not a historical execution report.
See [runtime architecture](runtime-architecture.md) and the library READMEs.

| Area | Remaining concern | Ownership |
|---|---|---|
| Generated ABI | Globals and incomplete extended CPU state transport | Lifter/project adapter |
| Guest memory | Raw generated accesses bypass checked helpers | Codegen/runtime with project layout policy |
| PE loading | Structural checks do not prove full binary identity | Revision validation/loader |
| Imports | Pointer widths, signatures, cleanup and errors | ABI/compatibility adapter |
| Windows services | Incomplete API subsets and legacy placeholders | Compatibility boundary |
| Host integration | Real window/input/audio/device tests still needed | Platform boundary |
| Diagnostics | Counts do not prove behavior | Project observer and validation |

Fix one demonstrated contract at a time. Add synthetic tests for generic fixes
and preserve natural frontier evidence. Never move handwritten game behavior into
shared libraries. Old pre-extraction measurements remain in Git history.
