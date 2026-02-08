# Changelog

## 2026-02-08

- Locked runtime ownership in `docs/runtime-ownership.md`.
- Published contract baseline `runtime-contract-v0.1`.
- Expanded VM opcode/runtime slice and conformance tests.
- Added P0 parity opcodes (`Less*`, `Greater*`, `Equal*`, `I2F`, `F2I`, `I2Frac`, `Frac2I`) and `vm_extended_ops_test`.
- Added P1 numeric parity opcodes (`FAdd`, `FSub`, `FMul`, `FDiv`, `FracAdd`, `FracSub`, `FracMul`, `FracDiv`) and `vm_float_fraction_ops_test`.
- Added P2 structured value parity opcodes (`MakeOption*`, `Option*`, `MakeResult*`, `Result*`, `MakeEnum*`, `Enum*`) and `structured_values_test`.
- Added Axion parity hooks (`AxRead`, `AxSet`, `AxVerify`) and ternary logical ops (`TNot`, `TAnd`, `TOr`, `TXor`) with `vm_extended_ops_test` coverage.
- Added tensor/runtime parity opcodes (`TVecAdd`, `TMatMul`, `TTenDot`, `TVecMul`, `TTranspose`, `TExp`, `TSqrt`, `TSiLU`, `TSoftmax`, `TRMSNorm`, `TRoPE`, `ChkShape`, `WeightsLoad`, `SetF`) and `vm_tensor_test`.
- Added deterministic trap payload schema output (`TRAP_PAYLOAD ...`) and trace write-delta annotations in VM trace output.
- Updated runtime contract artifact to `contract_version=2026-02-08-v2` with explicit `trap_payload_contract`.
- Added runtime contract release checklist (`docs/release-checklist.md`) for `runtime-contract-v0.2` preparation.
- Added deterministic runtime perf regression gate (`make perf-check`) with CI artifact upload and baseline contract.
- Added loader fuzz smoke coverage (`vm_loader_fuzz_smoke_test`) for hardening.
- Added preview execution mode plumbing (`--mode accelerated-preview`) with explicit mode parity check (`scripts/check-mode-parity.sh`).
- Added acceleration feature-gating RFC (`docs/rfcs/RFC-0001-acceleration-feature-gating.md`) and execution-mode contract metadata.
- Promoted runtime contract baseline preparation to `runtime-contract-v0.3` with `contract_version=2026-02-08-v3` (no host ABI break; `host_abi.version` remains `0.1.0`).
- Added host C ABI (`include/t81/vm/c_api.h`) and shared/static artifacts.
- Added cross-repo CI (`.github/workflows/lang-compat.yml`) and ecosystem canary workflow support.
