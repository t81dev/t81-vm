# src/vm

Implementation namespace for HanoiVM runtime components.

Current modules:

- `loader.cpp`: program image loading and policy extraction
- `program_io.cpp`: file artifact parsing (`.t81vm`, `.tisc.json`)
- `validator.cpp`: static program validation checks
- `vm.cpp`: deterministic interpreter implementation
- `summary.cpp`: deterministic snapshot and state hash helpers
- `main.cpp`: CLI runner used by harness (`build/t81vm`)

Next modules:

- `memory`
- `abi`
