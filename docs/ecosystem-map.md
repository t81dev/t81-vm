# Ecosystem Map

This document describes how `t81-vm` relates to repositories under <https://github.com/t81dev>.

## Direct Dependencies (Normative/Input)

- `t81-foundation`: canonical model, instruction constraints, safety invariants.
- `duotronic-whitepaper`: formal semantics context and terminology.
- `t81-lang`: source of TISC bytecode and compiler metadata consumed by the VM.

## Direct Consumers (Output/Runtime)

- `t81-python`: VM embedding surface and runtime control APIs.
- `t81-examples`: executable programs and tutorial workloads.
- `t81-benchmarks`: conformance and performance harnesses.

## Coordination Repositories

- `t81-roadmap`: milestone alignment and delivery communication.
- `t81-docs`: user-facing guides that should reference this repo's stable contracts.
- `t81-constraints`: explicit failure boundaries and epistemic assumptions.

## Adjacent Research/Integration Tracks

- `t81lib`, `ternary`, `llama.cpp`: ternary quantization and runtime integration possibilities.
- `t81-hardware`, `ternary-fabric`: future hardware execution targets for compatible semantics.
- `trinity`, `trinity-pow`, `trinity-decrypt`: potential consumers of deterministic VM execution primitives.

## Artifact Contracts

`t81-vm` should publish:

- Bytecode compatibility matrix (`spec_version` support).
- Conformance vector bundle (pass/fail + canonical hashes).
- Stable trap code registry.
- Host ABI signature and lifecycle contract.

These outputs are expected inputs for documentation, language bindings, and benchmark suites.
