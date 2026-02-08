# Runnable Examples

These artifacts are executable by `build/t81vm` and serve as cross-repo compatibility anchors.

## Supported Program Formats

1. `Text V1` (`*.t81vm`)
   - One instruction per line: `<OPCODE> <a> <b> <c>`
   - Optional policy line: `POLICY <policy-text>`
2. `TISC JSON V1` (`*.tisc.json`)
   - JSON object containing `insns` array of instruction objects:
     - `opcode` (string)
     - `a`, `b`, `c` (integers)
   - Optional `axion_policy_text`.

## Quick Run

```bash
build/t81vm --trace --snapshot examples/runnable/arithmetic.t81vm
build/t81vm --trace --snapshot examples/runnable/arithmetic.tisc.json
```
