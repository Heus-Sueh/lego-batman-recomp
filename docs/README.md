# Documentation guide

Use `just frontier` for current execution observations. A bounded run ends at its
frame limit; it does not necessarily expose the next blocker. The milestone model
lives in [recomp-project](../.agents/skills/recomp-project/SKILL.md); evidence rules
live in [recomp-validation](../.agents/skills/recomp-validation/SKILL.md).

## Active reading paths

| Need | Canonical source |
|---|---|
| Runtime ownership, migrated services, limitations and tests | [Runtime architecture](runtime-architecture.md) |
| Why the runtime migration was needed | [Pre-migration audit](runtime-architecture-audit.md) |
| Current file-API migration target | [Next runtime target](implementation/file-api-migration.md) |
| Project-specific implementation workflow | [Recurring recipes](implementation/recurring-recipes.md) |
| Purge discovery implementation and limitations | [Stack purge discovery](implementation/stack-purge-discovery.md) |
| Required project behavior | [AGENTS](../AGENTS.md), [ADR 0001](decisions/0001-recompilation-vs-recreation.md), [ADR 0002](decisions/0002-decoupled-presentation-pacing.md) |
| Skill selection | [Capability index](skill-index.md) |
| Why a document is retained or superseded | [Documentation review](documentation-review.md) |

`architecture/` describes subsystem context and explicitly labeled proposals.
`gotchas/` records pitfalls; verify revision-specific addresses before applying
them. `references/` describes external tooling, not current game progress.
`investigations/` preserves historical experiments, including rejected approaches.
Old M/G milestone numbers must not be mapped directly onto today's gates.

Do not infer functioning gameplay, audio, streaming or visual equivalence from
a historical report, a passing synthetic test, or a presentation counter.
