# t81-vm Architecture

## Objectives

- Deterministic execution of TISC bytecode.
- Balanced-ternary-aware arithmetic and state management.
- Stable host boundary for language bindings and tooling.

## Runtime Pipeline

1. **Loader**: reads bytecode package, metadata, and declared `spec_version`.
2. **Validator**: checks instruction format, control-flow integrity, and operand domains.
3. **State Init**: allocates registers, memory regions, stack frames, and execution context.
4. **Interpreter Core**: fetch/decode/execute loop with deterministic scheduling.
5. **Trap Manager**: central trap creation and canonical serialization.
6. **Reporter**: emits stable execution summary and state hash.

## Core Subsystems

- **Bytecode Decoder**: canonical parsing, endian normalization, version checks.
- **Instruction Engine**: opcode dispatch and arithmetic core.
- **Memory Model**: segmented regions, strict bounds checks, deterministic allocation policy.
- **Determinism Layer**: canonical ordering, stable hashing, reproducible trap policy.
- **Host ABI Bridge**: explicit narrow interface for embedding/runtime API consumers.

## Determinism Contract

The runtime must satisfy deterministic execution contract constraints:

- Same bytecode + same initial state + same policy inputs => same outputs and traces.
- Trap classes and trap ordering remain stable across platforms.
- Scheduler choices (if concurrency is enabled) are explicit and replayable.
- No hidden entropy, wall-clock, or host nondeterminism in execution semantics.

## Memory and Fault Invariants

- All accesses resolve to a valid logical segment or trap deterministically.
- State transitions are committed atomically per instruction.
- Trace entries include fault context (`pc`, opcode, trap code).
- Policy/Axion metadata is attached at program load and remains observable at fault/exit.

## Non-Goals (Initial Phases)

- JIT compilation.
- Parallel speculative execution.
- Platform-specific performance shortcuts that alter behavior.

## Extensibility Points

- Opcode family expansion via version-gated decoder tables.
- Optional profiling hooks behind non-observable channels.
- Alternate host embedding layers preserving ABI contract.
