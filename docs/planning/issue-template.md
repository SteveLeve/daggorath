# Issue shape

Phase issues and work-item issues use this body so an agent can take one as its
prompt. The phase issue links the prompt file; work items are sub-issues.

```markdown
## Goal
One sentence: the behaviour or artifact that exists when this is done.

## Read first
- docs/prompts/phase-N-….md (phase issues)
- the specification sections and ADRs this touches

## Scope
In: …
Out (do not build): …

## Evidence
Listing routines to read; fixtures to add or extend; labels expected.

## Acceptance
- [ ] concrete, checkable outcomes, each tied to a test, fixture or document
- [ ] `make all` passes; output pasted in the PR

## Review
docs/planning/review-checklist.md
```

Labels: `phase-N`, `track-r`, `adr`, `spec`, `core`, `fixtures`, `blocked`.
