# VM Parity Backlog

Snapshot date: 2026-02-08

This backlog tracks explicit parity gaps between `t81-vm` and the upstream runtime surface in `t81-foundation`.

## Current Delta

- Opcode coverage: `47` (`t81-vm`) vs `81` (`t81-foundation`)
- VM conformance tests (`vm*_test.cpp`): `11` (`t81-vm`) vs `13` (`t81-foundation`)

## Priority Burn-Down Order (By Ecosystem Impact)

### P0: Consumer Stability (`t81-lang` / `t81-python` blockers)

- [x] Comparison ops: `Less`, `LessEqual`, `Greater`, `GreaterEqual`, `Equal`, `NotEqual`
- [x] Numeric conversions: `I2F`, `F2I`, `I2Frac`, `Frac2I`
- [x] Port `vm_extended_ops_test.cpp`

### P1: Deterministic Numeric Runtime Coverage

- [x] Float/fraction arithmetic: `FAdd`, `FSub`, `FMul`, `FDiv`, `FracAdd`, `FracSub`, `FracMul`, `FracDiv`
- [x] Port `vm_float_fraction_ops_test.cpp`

### P2: Structured Values + Language Surface Completion

- [ ] Structured value ops: `MakeOption*`, `Option*`, `MakeResult*`, `Result*`, `MakeEnum*`, `Enum*`

### P3: Tensor and Axion Extensions

- [ ] Tensor ops: `TVecAdd`, `TMatMul`, `TTenDot`, `TVecMul`, `TTranspose`, `TExp`, `TSqrt`, `TSiLU`, `TSoftmax`, `TRMSNorm`, `TRoPE`, `ChkShape`
- [ ] Axion hooks: `AxRead`, `AxSet`, `AxVerify`
- [ ] Port `vm_tensor_test.cpp`

### P4: Remaining Logical/Runtime Extensions

- [ ] Ternary logical ops: `TNot`, `TAnd`, `TOr`, `TXor`
- [ ] Runtime extensions: `WeightsLoad`, `SetF`

## Pending Opcode Families

1. Ternary logical ops: `TNot`, `TAnd`, `TOr`, `TXor`
2. Axion hooks: `AxRead`, `AxSet`, `AxVerify`
3. Tensor ops: `TVecAdd`, `TMatMul`, `TTenDot`, `TVecMul`, `TTranspose`, `TExp`, `TSqrt`, `TSiLU`, `TSoftmax`, `TRMSNorm`, `TRoPE`, `ChkShape`
4. Structured value ops: `MakeOption*`, `Option*`, `MakeResult*`, `Result*`, `MakeEnum*`, `Enum*`
5. Runtime extensions: `WeightsLoad`, `SetF`

## Pending VM Tests To Port

1. `vm_tensor_test.cpp`

## Exit Criteria For Parity Phase

1. Contract artifact lists complete supported opcode set.
2. All upstream `vm*_test.cpp` classes are represented in `t81-vm` suites.
3. `t81-lang` compatibility gate validates no contract regressions.
4. `t81-python` canary passes in both pinned and floating integration lanes.

## Tracking Rule

- Prioritize items strictly in P0 -> P4 order unless an explicit compatibility incident requires reprioritization.
