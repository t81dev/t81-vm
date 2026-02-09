# RFC-0002: Runtime-Contract-v0.5 Promotion Scope

Status: Draft  
Owner: `t81-vm` maintainers  
Last updated: 2026-02-08

## Context

`runtime-contract-v0.4` established execution-mode parity evidence metadata and promotion discipline.
The next baseline (`runtime-contract-v0.5`) should harden those guarantees into explicit cross-repo governance obligations that downstream repos can validate automatically.

## Goals

1. Add machine-readable parity expectations for execution modes.
2. Make promotion evidence requirements explicit in the contract release flow.
3. Preserve host ABI compatibility (`host_abi.version` remains `0.1.x`) unless a deliberate ABI change is proposed.

## Non-Goals

1. Introducing a new host ABI major version.
2. Declaring `accelerated-preview` as stable in this cycle.
3. Expanding opcode surface solely for this promotion.

## Proposed v0.5 Delta

1. Extend `docs/contracts/vm-compatibility.json` with a parity evidence section for execution modes.
2. Require deterministic parity checks against canonical vectors in CI for both pinned and floating lanes.
3. Record v0.5 evidence links in:
   - `t81-roadmap/MIGRATION_DASHBOARD.md`
   - `t81-roadmap/MIGRATION_CHECKPOINTS.md`
   - `t81-docs/docs/runtime-contract.md`

## Acceptance Criteria

1. `t81-vm` contract artifact has `contract_version` bumped to the v0.5 value.
2. `scripts/check-vm-contract.py` validates the new parity evidence fields.
3. `t81-vm` `ci`, `lang-compat`, and `ecosystem-contract` workflows are green.
4. Downstream docs and pinned-lane references are updated in `t81-lang`, `t81-python`, and `t81-docs`.
5. Promotion checkpoint is logged in `t81-roadmap` with commit pins and workflow URLs.

## Open Questions

1. Should parity evidence require equality at intermediate checkpoints or only terminal `STATE_HASH`?
2. Which vector set is mandatory for v0.5 parity proof (`arithmetic`, `faults`, tensor fault chain)?
