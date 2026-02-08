# Runtime Ownership

Snapshot date: 2026-02-08

This document freezes ownership boundaries for the VM migration window.

## Owner Of Runtime Execution Contract

- Canonical runtime owner: `t81-vm`
- Upstream normative reference: `t81-foundation/spec/t81vm-spec.md`
- Language producer of VM inputs: `t81-lang`
- Primary embedding consumer: `t81-python`

## Scope Owned By `t81-vm`

- VM instruction decode/validation behavior.
- Interpreter execution semantics and deterministic trap behavior.
- Trap code registry and serialization contract.
- Deterministic trace and snapshot hash contract.
- Host embedding ABI (`include/t81/vm/c_api.h`).

## Scope Not Owned By `t81-vm`

- Language grammar and source-level type semantics (`t81-lang`).
- Axion policy language definition (`t81-foundation` RFC/spec source).
- High-level docs narrative and onboarding (`t81-docs`).

## Compatibility Rule

Contract-impacting changes in any of these surfaces require:

1. Contract artifact update in `docs/contracts/vm-compatibility.json`.
2. Compatibility note update in `docs/ecosystem-map.md`.
3. Cross-repo acknowledgement in `t81-lang` compatibility docs and checks.
