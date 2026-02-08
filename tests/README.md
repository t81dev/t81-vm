# tests

Deterministic conformance suites for T81VM.

Current suites:

- `tests/cpp/*_test.cpp`: VM behavior and trap regression tests.
- `tests/harness/harness.py`: replay-hash determinism and fault-vector checks.

Highlighted VM migration suites:

- `vm_load_store_test.cpp`
- `vm_memory_test.cpp`
- `vm_jump_flags_test.cpp`
- `vm_bounds_trace_test.cpp`
- `vm_neg_jumps_test.cpp`
- `vm_tensor_test.cpp`

Planned expansions:

- richer bytecode decode/validation fixtures
- trap payload schema checks
