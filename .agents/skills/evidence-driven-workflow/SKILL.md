---
name: evidence-driven-workflow
description: Guide complex tasks with explicit evidence, milestones, validation, and durable knowledge capture. Use when investigating an unfamiliar system, planning or implementing a multi-step change, reviewing work, making decisions under uncertainty, or handing work between sessions or agents.
---

# Evidence-Driven Workflow

Use this skill to turn ambiguous work into a sequence of verifiable decisions and small, reversible increments. Apply it across domains: software, research, design, writing, data, operations, and project planning.

## Operating principles

- Establish the desired outcome and the smallest useful deliverable before acting.
- Prefer direct evidence over narrative, assumptions, or apparent progress.
- Keep domain-specific facts in the project’s own documentation; keep this skill procedural.
- Separate discovery, decision, implementation, and validation.
- Make uncertainty visible instead of filling gaps with confident guesses.
- Prefer the smallest change that proves or disproves the current hypothesis.

## Workflow

### 1. Frame the task

State the objective, user-visible outcome, scope, constraints, dependencies, risks, current state, known unknowns, definition of done, and next observable milestone. If a missing choice materially changes the solution, ask a focused question. Otherwise choose a reversible default and record it.

### 2. Inspect before designing

Read authoritative local sources first: existing implementation, tests, configuration, data, logs, specifications, and project instructions. Search for existing patterns and working references before inventing abstractions.

Build a compact inventory of what exists, what is proven, what is incomplete, required interfaces, and competing explanations. Do not treat a task description, README, TODO, or issue title as proof of implementation state.

### 3. Form a plan and gates

Break work into milestones that each produce an observable result. For every milestone define the action, expected evidence, validation method, failure or rollback boundary, and whether it unblocks the next milestone. Do not advance through a gate until its evidence is reproducible.

For substantial capabilities or new subsystem contracts, capture the requirements in a falsifiable specification (`specs/`) before task execution. Keep small, isolated bug fixes spec-free (task + minimal reproduction + test). When runtime evidence contradicts an assumption in the specification, revise the spec instead of forcing the implementation to satisfy an invalid assumption.

### 4. Execute incrementally

Make one coherent change at a time. Preserve working behavior, avoid unrelated refactors, and run the narrowest useful check after each meaningful change. When approaches compete, test the cheapest approach that can distinguish them.

Use existing tools, APIs, examples, and project conventions before adding dependencies or abstractions. Keep changes reversible and explain deviations from established patterns.

### 5. Validate the outcome

Validate at the level of the user’s actual goal, not merely code activity. Use appropriate evidence: tests, builds, type checks, rendered output, measurements, traces, manual inspection, reference comparison, or independent review.

Report what was validated, the exact command or observation used, what remains unverified, and limitations. Never weaken a test, hide an error, or mark a milestone complete merely to obtain a green result.

## Evidence classification

Label important findings:

- **VERIFIED** — directly reproduced or observed from an authoritative source;
- **PARTIALLY VERIFIED** — some required behavior is proven, but meaningful parts remain untested;
- **INFERRED** — best explanation supported by evidence but not directly demonstrated;
- **HYPOTHESIS** — proposed explanation or next experiment;
- **UNKNOWN** — insufficient evidence.

Prefer evidence in this order, adapting to the domain: direct behavior or primary artifact; reproducible tests, traces, logs, or rendered results; existing implementation and configuration; trusted references or specifications; project documentation; assumptions and memory.

Do not present an inference or hypothesis as fact. When evidence conflicts, record the conflict and design the next discriminating check.

## Reference-first strategy

Before creating a new solution, find the closest working local or external example; confirm it solves the same class of problem; trace its assumptions and boundaries; reuse the proven pattern where appropriate; and document intentional differences. Verify compatibility, current behavior, licensing, and project constraints before copying or adapting anything.

## Failure handling

When a step fails:

1. Preserve the failure output and current state.
2. Classify the failure as environment, input, integration, implementation, specification, or unknown.
3. Reduce it to the smallest reproducible case.
4. Update the hypothesis and run the smallest experiment that distinguishes causes.
5. Stop and escalate when proceeding would destroy evidence, broaden scope materially, or conceal uncertainty.

Do not stack speculative fixes, retry blindly, or silently route around unsupported behavior.

## Anti-patterns

Do not confuse activity with progress; implement before understanding the system; invent requirements; silently choose consequential behavior; introduce speculative abstractions; bundle unrelated refactors; duplicate existing capabilities without reason; weaken validation; delete failure evidence; claim completion from partial success; mix separate failure domains without isolating them; leave important knowledge only in chat history; or put project-specific facts into this reusable procedure.

## Knowledge capture

Before finishing, update the project’s existing documentation structure, or create the smallest appropriate record if none exists. Capture verified discoveries and evidence, decisions and rationale, failed approaches, unresolved questions, next experiments, changed interfaces or assumptions, current milestone status, and exact validation results.

Use project conventions. Typical destinations include `AGENTS.md` for repository rules, `docs/research/` for investigations, `docs/decisions/` for choices, `docs/gotchas.md` for traps, and implementation notes for current status. Do not create a documentation tree merely to follow these names.

## Completion criteria

Consider the task complete only when the requested outcome exists within scope, the relevant gate has reproducible evidence, validation covers acceptance criteria, failures and limitations are disclosed, no unrelated work is bundled in, durable knowledge is captured, and the next useful action is clear if work remains blocked or partial.

When reporting, lead with the outcome, then provide concise evidence, remaining uncertainty, and follow-up actions.
