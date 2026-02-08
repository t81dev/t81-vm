# RFC-0001: Compatibility-Safe Acceleration Feature Gating

Status: Draft  
Owner: `t81-vm` maintainers  
Last updated: 2026-02-08

## Context

`t81-vm` now has a tagged contract baseline (`runtime-contract-v0.2`) and closed opcode parity against `t81-foundation`.
Phase 4 work requires acceleration experiments, but deterministic behavior and contract compatibility are non-negotiable.

## Goals

1. Allow experimental acceleration modes without changing default interpreter behavior.
2. Preserve deterministic output contracts (`STATE_HASH`, trap payload, trace schema) across modes.
3. Make compatibility risk explicit and testable in CI.

## Non-Goals

1. Defining a specific acceleration backend in this RFC.
2. Relaxing contract requirements for default runtime mode.

## Proposed Model

Introduce explicit execution modes:

- `interpreter` (default, contract baseline)
- `accelerated-preview` (opt-in, gated)

Rules:

1. Default CLI/API behavior remains `interpreter`.
2. `accelerated-preview` must be opt-in via explicit flag or API parameter.
3. Any `accelerated-preview` divergence from `interpreter` must be detectable by contract checks:
   - mismatch in `STATE_HASH`
   - mismatch in trap class/payload
   - mismatch in required trace schema fields

## Gating Requirements

Before enabling any new acceleration path in CI:

1. Add side-by-side execution test vectors (`interpreter` vs `accelerated-preview`).
2. Add deterministic replay checks for both pinned and floating downstream lanes.
3. Record performance claims in `t81-benchmarks` with reproducibility profile IDs.
4. Document mode-specific limitations in `docs/contracts/vm-compatibility.json`.

## Rollout Plan

1. Land mode plumbing behind disabled defaults.
2. Add CI lane for `accelerated-preview` as non-blocking.
3. Require sustained parity/perf evidence before promoting any lane to blocking.

## Open Questions

1. Should `accelerated-preview` state hashes be required to match exactly at every checkpoint or only at halt?
2. Which tensor-heavy vectors should be canonical for mode-parity promotion gates?
