# T81VM Specification (Draft)

## 1. Scope

This specification defines runtime behavior for executing TISC bytecode in a deterministic, balanced-ternary-aware virtual machine.

Normative upstream references:

- `t81-foundation/spec/t81vm-spec.md`
- `t81-foundation/spec/tisc-spec.md`
- `t81-foundation/spec/rfcs/RFC-0002-deterministic-execution-contract.md`
- `duotronic-whitepaper`

If conflicts exist, the `t81-foundation` normative spec set prevails.

## 2. Determinism Requirements

A conforming VM implementation MUST:

- Produce identical observable results for identical bytecode and identical inputs.
- Reject malformed bytecode before execution begins.
- Use architecture-independent integer and trit semantics.
- Emit traps with stable codes and stable ordering.
- Avoid undefined behavior in arithmetic, memory, and control flow.

Observable outputs include:

- process exit status
- final state hash
- trap code and payload
- canonical execution trace

## 3. Execution Modes

Implementations MUST support interpreter mode and MAY support deterministic JIT mode.

### 3.1 Interpreter Mode

Interpreter mode is the reference behavior:

1. Fetch instruction at `PC`.
2. Decode and validate operands.
3. Execute opcode semantics.
4. Commit state transition.
5. Append trace event.

### 3.2 Deterministic JIT Mode

A JIT backend is conformant only if it preserves interpreter-observable behavior:

- identical register/memory outcomes
- identical trap class and ordering
- identical Axion-visible event ordering

### 3.3 Lifecycle

Execution lifecycle is:

1. Decode bytecode stream.
2. Validate program structure and operand domains.
3. Initialize deterministic machine state.
4. Execute until halt or trap.
5. Emit canonical summary.

## 4. Concurrency Model

Concurrency is optional. If implemented, it MUST be deterministic.

Requirements:

- Scheduling policy MUST be explicitly defined and replayable.
- Inter-context communication MUST use deterministic channels or ownership-defined memory regions.
- Any conflict MUST resolve through deterministic rules or deterministic trap.
- Concurrency MUST NOT change final observable behavior for the same initial state.

## 5. Memory Model

### 5.1 Logical Segments

VM memory uses logical segments:

- `CODE` (read-only executable content)
- `STACK` (frame-scoped temporaries and returns)
- `HEAP` (dynamic objects)
- `TENSOR` (tensor/matrix payload region)
- `META` (trace/policy metadata)

### 5.2 Addressing and Bounds

Implementations MUST map every address to either one valid segment or invalid.

- Invalid or out-of-segment access MUST trap deterministically.
- Address wraparound is forbidden.
- Bounds failures MUST be reproducible in trace output.

### 5.3 Canonical Value Storage

Values committed to machine state MUST be canonical under the applicable data-type rules.

- Non-canonical external values MUST be normalized before commit, or rejected.
- Canonicalization behavior MUST be deterministic and testable.

### 5.4 Deterministic GC/Compaction

If GC/compaction exists:

- collection points and effects MUST be deterministic for identical runs
- relocation events MUST be represented in trace metadata
- object identity MUST remain semantically stable

## 6. Safety Boundaries

The VM MUST enforce:

- bounded memory regions with explicit access checks
- overflow/division policy per opcode contract
- deterministic trap-on-invalid-state semantics
- no host pointer aliasing into guest-visible state

## 7. Axion Interface

The VM MUST expose a deterministic observation surface for Axion/policy engines.

Minimum interface guarantees:

- policy text or policy artifact binding at program load
- deterministic event ordering for instruction/segment/fault events
- final state snapshot on halt or trap
- stable reason strings for policy-relevant trap categories

## 8. Trap Semantics and Fault State

Minimum trap classes:

- `TRAP_DECODE`
- `TRAP_VALIDATION`
- `TRAP_ARITH_OVERFLOW`
- `TRAP_DIVISION`
- `TRAP_MEM_BOUNDS`
- `TRAP_ILLEGAL_OPCODE`
- `TRAP_HOST_ABI`

Fault handling requirements:

- Traps MUST be serialized in canonical form.
- Trap payload shape MUST be versioned.
- State-at-fault visibility (`PC`, opcode, operand context) MUST be available for replay.
- Trap order for equivalent failing inputs MUST be identical.

## 9. Conformance

A build is conformant when:

- deterministic replay tests pass (`run twice -> identical trace hash`)
- VM fault behavior tests pass for decode, bounds, division, and illegal instruction paths
- trap ordering tests match expected fixtures
- canonical summary output remains stable across supported host platforms

## 10. Versioning

VM behavior is versioned independently from implementation internals.

- `spec_version`: normative behavior contract
- `engine_version`: implementation release identifier

Bytecode declares minimum required `spec_version`.
