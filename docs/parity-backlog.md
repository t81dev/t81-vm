# VM Parity Backlog

Snapshot date: 2026-02-08

This backlog tracks explicit parity gaps between `t81-vm` and the upstream runtime surface in `t81-foundation`.

## Current Delta

- Opcode coverage: `29` (`t81-vm`) vs `81` (`t81-foundation`)
- VM conformance tests (`vm*_test.cpp`): `10` (`t81-vm`) vs `13` (`t81-foundation`)

## Pending Opcode Families

1. Ternary logical ops: `TNot`, `TAnd`, `TOr`, `TXor`
2. Axion hooks: `AxRead`, `AxSet`, `AxVerify`
3. Numeric conversions: `I2F`, `F2I`, `I2Frac`, `Frac2I`
4. Float/fraction arithmetic: `FAdd`, `FSub`, `FMul`, `FDiv`, `FracAdd`, `FracSub`, `FracMul`, `FracDiv`
5. Tensor ops: `TVecAdd`, `TMatMul`, `TTenDot`, `TVecMul`, `TTranspose`, `TExp`, `TSqrt`, `TSiLU`, `TSoftmax`, `TRMSNorm`, `TRoPE`, `ChkShape`
6. Structured value ops: `MakeOption*`, `Option*`, `MakeResult*`, `Result*`, `MakeEnum*`, `Enum*`
7. Comparison ops: `Less`, `LessEqual`, `Greater`, `GreaterEqual`, `Equal`, `NotEqual`
8. Runtime extensions: `WeightsLoad`, `SetF`

## Pending VM Tests To Port

1. `vm_extended_ops_test.cpp`
2. `vm_float_fraction_ops_test.cpp`
3. `vm_tensor_test.cpp`

## Exit Criteria For Parity Phase

1. Contract artifact lists complete supported opcode set.
2. All upstream `vm*_test.cpp` classes are represented in `t81-vm` suites.
3. `t81-lang` compatibility gate validates no contract regressions.
4. `t81-python` canary passes in both pinned and floating integration lanes.
