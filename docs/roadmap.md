# t81-vm Roadmap

## Phase 0: Contract Freeze

- Finalize `SPEC.md` with `t81-foundation` alignment.
- Define bytecode validation requirements and trap registry.
- Publish first conformance test vector format.

Exit criteria:

- Spec review complete.
- Initial fixture schema accepted.

## Phase 1: Deterministic Interpreter (MVP)

- Implement loader, validator, and interpreter loop.
- Support core arithmetic, control flow, and memory ops.
- Add deterministic replay tests.

Exit criteria:

- Stable deterministic execution across supported host platforms.
- Canonical state hash produced for every run.

## Phase 2: Host ABI + Tooling

- Expose stable embedding API for `t81-python`.
- Add execution traces and trap diagnostics.
- Stabilize `TRAP_PAYLOAD` summary schema and trace write-delta annotations.
- Add package-level bytecode compatibility checks.
- Add cross-repo compatibility CI gate with `t81-lang`.
- Add ecosystem contract fan-out CI (`ecosystem-contract.yml`) across `t81-lang`, `t81-python`, and `t81-docs`.
- Add end-to-end ecosystem canary (`t81-lang` -> `t81-vm` -> `t81-python`).
- Publish runtime contract release checklist for `runtime-contract-v0.2` preparation.

Exit criteria:

- First external embedding uses stable ABI.
- Tooling validates real-world examples.

## Phase 3: Performance and Hardening

- Optimize interpreter dispatch without changing behavior.
- Expand conformance + fuzz corpus.
- Introduce benchmark integration with `t81-benchmarks`.

Exit criteria:

- Measurable performance gains with zero conformance regressions.
- Reproducibility guarantees preserved.

## Phase 4: Advanced Execution Modes

- Explore compatibility-safe acceleration paths.
- Evaluate hardware co-targeting with `t81-hardware`.
- Define long-term feature gating model.

Exit criteria:

- Feature flags and behavior gating documented.
- Advanced mode contract accepted upstream.
