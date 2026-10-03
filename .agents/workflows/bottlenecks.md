---
description: Analyze how work actually moves through the project.
---

# Workflow Bottleneck Audit

Analyze how work actually moves through the project.

Goal:

Find steps that cause repeated work, waiting, rediscovery, debugging loops,
large validation gaps, or unnecessary agent effort.

## Map the Workflow

Identify the real path from:

request
→ discovery
→ research
→ implementation
→ build
→ test
→ runtime validation
→ documentation
→ completion

For each stage identify:

- inputs
- outputs
- dependencies
- manual operations
- validation
- common failures

## Detect Bottlenecks

Look for:

- repeated investigations
- repeated agent rediscovery
- long feedback loops
- repeated build failures
- repeated runtime failures
- missing intermediate validation
- manual repetitive operations
- missing scripts
- insufficient logging
- poor observability
- oversized tasks
- hidden dependencies
- unclear completion criteria
- documentation difficult to discover
- conflicting rules or skills

## Classify

Every finding must be:

VERIFIED
SUSPECTED
NEEDS_MEASUREMENT

Do not present speculation as a demonstrated bottleneck.

## Recommend

For each verified bottleneck provide:

Problem
Evidence
Current workflow
Root cause
Proposed improvement
Expected workflow
Validation method

Prefer improvements that shorten feedback loops.

Do not automatically rewrite rules or skills.

Use:

OBSERVE
→ PROPOSE
→ VALIDATE
→ ADOPT

Persist important findings under docs/workflow/.