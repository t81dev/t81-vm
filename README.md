# t81-vm

T81VM (HanoiVM) is the deterministic, balanced-ternary native virtual machine for the T81 ecosystem.

It executes TISC bytecode with strict reproducibility guarantees, overflow-safe arithmetic behavior, and explicit trap semantics so higher layers can reason about correctness.

## Current Status

`Draft architecture/specification + scaffold`

The repository currently defines execution contracts, boundaries, and milestone sequencing for implementation.

## Ecosystem Position

`t81-vm` is the runtime center of the stack:

- Consumes TISC bytecode from `t81-lang`
- Tracks normative semantics from `t81-foundation` and `duotronic-whitepaper`
- Exposes host/runtime surfaces used by `t81-python`
- Provides executable targets used by `t81-examples` and `t81-benchmarks`
- Aligns long-range goals with `t81-roadmap`

See `docs/ecosystem-map.md` for interface-level detail.

## Repository Layout

- `SPEC.md` - normative VM contract and deterministic execution rules
- `docs/architecture.md` - runtime architecture and subsystem boundaries
- `docs/ecosystem-map.md` - cross-repo artifact and dependency mapping
- `docs/runtime-ownership.md` - runtime ownership freeze and boundary rules
- `docs/contracts/vm-compatibility.json` - machine-readable compatibility contract
- `docs/parity-backlog.md` - explicit remaining parity delta vs `t81-foundation`
- `docs/roadmap.md` - phased delivery plan
- `docs/foundation-migration.md` - targeted migration plan from `t81-foundation`
- `src/vm/` - implementation entrypoint for the HanoiVM runtime
- `tests/` - deterministic conformance and regression suites

## Quickstart

```bash
make check
```

`make check` validates docs, builds the VM, runs C++ regression tests, and runs deterministic replay harness checks.

Cross-repo canary check:

```bash
make canary-check
```

CLI example:

```bash
build/t81vm --trace --snapshot tests/harness/test_vectors/arithmetic.t81
```

Runnable example artifacts:

```bash
build/t81vm --trace --snapshot examples/runnable/arithmetic.t81vm
build/t81vm --trace --snapshot examples/runnable/arithmetic.tisc.json
```

Accepted input formats are defined in `SPEC.md` (`Text V1` and `TISC JSON V1`).

## Near-Term Priorities

1. Freeze bytecode decoding and validation rules.
2. Implement single-thread deterministic interpreter loop.
3. Ship golden conformance vectors and replay tests.
4. Stabilize host ABI boundaries for `t81-python` integration.

## Related Repositories

- <https://github.com/t81dev/t81-foundation>
- <https://github.com/t81dev/t81-lang>
- <https://github.com/t81dev/t81-python>
- <https://github.com/t81dev/t81-examples>
- <https://github.com/t81dev/t81-benchmarks>
- <https://github.com/t81dev/t81-roadmap>
- <https://github.com/t81dev/duotronic-whitepaper>

## License

Pending repository policy decision.
