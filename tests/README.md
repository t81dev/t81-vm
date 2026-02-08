# tests

Deterministic conformance suites for T81VM.

Current suites:

- `tests/cpp/*_test.cpp`: VM behavior and trap regression tests.
- `tests/harness/harness.py`: replay-hash determinism and fault-vector checks.

Planned expansions:

- richer bytecode decode/validation fixtures
- trap payload schema checks
