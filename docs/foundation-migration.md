# t81-foundation Migration Candidates for t81-vm

This document maps concrete assets in `t81dev/t81-foundation` to migration actions for `t81-vm`.

## Scope

- Source reviewed: `https://github.com/t81dev/t81-foundation` (`main`)
- Focus: VM-specific spec, API surface, traps/state, and conformance tests
- Goal: reuse stable contracts while keeping `t81-vm` narrowly scoped as the standalone VM repo

## Recommended Migrations

1. **Adopt spec structure from `spec/t81vm-spec.md` into `SPEC.md`**
   - Action: adapt (not copy verbatim)
   - Why: adds missing normative sections we currently do not define in detail
   - Add these sections in `SPEC.md`:
     - `Execution Modes` (interpreter and deterministic JIT behavior parity)
     - `Concurrency Model` (deterministic scheduling/ordering rules)
     - `Memory Model` (CODE/STACK/HEAP/TENSOR/META semantics)
     - `Axion Interface` (observable hooks and policy contract)
     - `Error and Fault Handling` (state-at-fault guarantees)

2. **Import deterministic contract requirements from `spec/rfcs/RFC-0002-deterministic-execution-contract.md`**
   - Action: adapt
   - Why: formalizes determinism obligations across state, traces, faults, and concurrency
   - Target: `SPEC.md` and `docs/architecture.md` determinism subsections

3. **Mirror VM public interface shape from `include/t81/vm/vm.hpp`**
   - Action: copy pattern, reimplement locally
   - Why: strong minimal abstraction (`load_program`, `step`, `run_to_halt`, state access) that works for interpreter-first development
   - Target: new headers under `src/vm/` once implementation begins

4. **Adopt trap taxonomy baseline from `include/t81/vm/traps.hpp`**
   - Action: adapt and version
   - Why: existing trap classes align with deterministic fault handling and VM tests
   - Target: trap registry in `SPEC.md`, then concrete enum in runtime code

5. **Adopt state model concepts from `include/t81/vm/state.hpp`**
   - Action: adapt selectively
   - Why: useful primitives for deterministic replay (`TraceEntry`, `ValueTag`, explicit flags/state containers)
   - Target: `src/vm/state.*` design and test fixtures

6. **Port VM conformance tests from `tests/cpp/vm_*_test.cpp`**
   - Action: incremental port
   - Why: immediately validates bounds, illegal opcodes, faults, trace behavior, and div/mod edge cases
   - Suggested first ports:
     - `tests/cpp/vm_bounds_test.cpp`
     - `tests/cpp/vm_fault_test.cpp`
     - `tests/cpp/vm_illegal_test.cpp`
     - `tests/cpp/vm_trace_test.cpp`
     - `tests/cpp/vm_divmod_test.cpp`

7. **Port test harness model from `tests/harness/`**
   - Action: adapt
   - Why: deterministic double-run hashing pattern is a direct fit for `t81-vm`
   - Target: add a local `tests/harness/` with vector replay and trace-hash checks

## Defer or Avoid for Now

1. **Do not migrate `src/vm/vm.cpp` wholesale (large monolith, ~1700+ LOC)**
   - Reason: high coupling to broader `t81-foundation` modules (`axion`, tensor, language tooling)
   - Better: reimplement in smaller components (`loader`, `validator`, `interpreter`, `traps`, `abi`)

2. **Do not migrate generated docs (`docs/api/html/**`)**
   - Reason: generated artifacts, not source-of-truth contracts

3. **Do not migrate `legacy/hanoivm/**`**
   - Reason: historical implementation track, likely drift from current contract

4. **Do not migrate unrelated stack layers yet (`canonfs`, cognition tiers, broad tensor stack)**
   - Reason: keep VM repo focused on execution core and deterministic contract

## First Migration Slice (practical)

1. Expand `SPEC.md` with `Execution Modes`, `Concurrency`, `Memory Model`, and `Axion Interface`.
2. Add VM interface stubs (`vm`, `state`, `traps`) in `src/vm/`.
3. Port five small VM tests listed above.
4. Add a deterministic replay harness (`run twice -> hash trace -> compare`).

This sequence gives contract depth plus immediate regression protection without importing monorepo complexity.

## Next Parity Slices

See `docs/parity-backlog.md` for the canonical gap tracker.

### Slice A (immediate)

1. Expand control-flow and stack op coverage for conditional branches and call/ret corner-cases.
2. Extend memory/segment trace verification for stack/heap/meta boundary events.
3. Keep `docs/contracts/vm-compatibility.json` in sync with supported opcode set.

### Slice B (after A is stable)

1. Port selected extended VM tests (`vm_extended_ops`, targeted `vm_memory` deep cases).
2. Add additional deterministic replay vectors from upstream runtime semantics.
3. Publish updated compatibility notes for downstream consumers (`t81-lang`, `t81-python`).
