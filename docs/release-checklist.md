# Runtime Contract Release Checklist

This checklist is the release discipline for VM contract and ABI updates.

## Scope

Use this checklist whenever changes touch:

- `include/t81/tisc/opcodes.hpp`
- `include/t81/vm/c_api.h`
- `include/t81/vm/vm.hpp`
- trap/trace output contracts in `src/vm/main.cpp` and `src/vm/summary.cpp`

## Required Steps

1. Update `docs/contracts/vm-compatibility.json`.
2. Update `CHANGELOG.md` with compatibility impact notes.
3. Run:
   - `make check`
   - `python3 scripts/check-vm-contract.py`
   - `scripts/check-contract-discipline.sh`
4. Run ecosystem verification:
   - `make canary-check`
5. Sync downstream baseline references:
   - `t81-lang/docs/architecture/compatibility-matrix.md`
   - `t81-python/docs/compatibility.md`
   - `t81-roadmap/MIGRATION_DASHBOARD.md`
   - `t81-docs/docs/runtime-contract.md`

## Runtime-Contract-v0.3 Status

- Current status: pending tag creation in this promotion cycle.
- Prior baseline: `runtime-contract-v0.2` tagged at `t81-vm` commit `30306b32eea8b7acde1be354ed10d24881989225`.
- Landed scope:
  - stable trap payload line (`TRAP_PAYLOAD ...`)
  - trace write-delta annotations (`:write=...`)
- Keep pinned/floating downstream lanes green before promoting the next contract tag.
