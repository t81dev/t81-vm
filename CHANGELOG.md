# Changelog

## 2026-02-08

- Locked runtime ownership in `docs/runtime-ownership.md`.
- Published contract baseline `runtime-contract-v0.1`.
- Expanded VM opcode/runtime slice and conformance tests.
- Added P0 parity opcodes (`Less*`, `Greater*`, `Equal*`, `I2F`, `F2I`, `I2Frac`, `Frac2I`) and `vm_extended_ops_test`.
- Added host C ABI (`include/t81/vm/c_api.h`) and shared/static artifacts.
- Added cross-repo CI (`.github/workflows/lang-compat.yml`) and ecosystem canary workflow support.
