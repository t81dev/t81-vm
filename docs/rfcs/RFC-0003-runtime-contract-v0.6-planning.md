# RFC-0003: Runtime-Contract-v0.6 Planning Scope

Status: Draft
Owner: `t81-vm` maintainers
Last updated: 2026-02-09
Roadmap tracker: https://github.com/t81dev/t81-roadmap/issues/24

## Context

`runtime-contract-v0.5` established cross-repo governance metadata, deterministic
fixture evidence requirements, and migration automation checks spanning
`t81-vm`, `t81-lang`, `t81-python`, `t81-docs`, `t81-examples`, and
`t81-foundation`.

The next planning cycle (`runtime-contract-v0.6`) should focus on incremental,
evidence-backed contract hardening without breaking host ABI compatibility.

## Goals

1. Define the minimum v0.6 delta before any promotion execution work starts.
2. Publish an explicit downstream impact checklist across all marker repos.
3. Preserve host ABI compatibility (`host_abi.version` remains `0.1.x`) unless
   a separate ABI-break RFC is accepted.

## Non-Goals

1. Introducing a host ABI major version bump.
2. Expanding opcode surface without corresponding conformance + downstream sync.
3. Declaring `accelerated-preview` stable without sustained parity evidence.

## Proposed v0.6 Planning Delta

1. Contract metadata:
   - clarify required promotion evidence fields and retention period.
   - keep deterministic fixture path and release artifact naming aligned to
     canonical `runtime-v0.5` paths unless superseded by an accepted RFC.
2. Promotion operations:
   - require one rehearsal record before any tag promotion.
   - require `t81-roadmap/docs/phase5-quality-gate-streak.md` status snapshot in
     promotion notes.
3. Downstream checklist publication:
   - publish checklist covering:
     - `t81-lang`
     - `t81-python`
     - `t81-docs`
     - `t81-examples`
     - `t81-foundation`
     - `t81-roadmap`

## Acceptance Criteria

1. v0.6 planning RFC is linked in roadmap tracker issue `#24`.
2. Downstream impact checklist exists and is referenced from roadmap status.
3. Contract validator updates (if any) remain green in `scripts/check-vm-contract.py`.
4. No host ABI major-version bump is introduced in this cycle.

## Open Questions

1. Should v0.6 require additional parity signals beyond `STATE_HASH`,
   `TRAP_CLASS`, and `TRAP_PAYLOAD`?
2. Should a golden-demo release artifact be mandatory for every promotion cut?
