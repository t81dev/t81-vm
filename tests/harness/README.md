# T81VM Determinism Harness

This harness performs deterministic replay checks by running each vector twice and comparing both:

- SHA-256 hash of full trace/snapshot output
- `STATE_HASH` emitted by VM snapshot output

## Running

```bash
python3 tests/harness/harness.py run
```

The harness expects `build/t81vm` to exist.
