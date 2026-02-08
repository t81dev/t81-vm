#!/usr/bin/env bash
set -euo pipefail

vm="build/t81vm"

run_ok() {
  local file="$1"
  echo "example ok: $file"
  local out
  out="$($vm --trace --snapshot "$file")"
  echo "$out" | grep -E '^STATE_HASH ' >/dev/null
}

run_fault() {
  local file="$1"
  echo "example fault: $file"
  set +e
  "$vm" --trace --snapshot "$file" >/tmp/t81vm_example_out.txt 2>/tmp/t81vm_example_err.txt
  local rc=$?
  set -e
  if [[ $rc -eq 0 ]]; then
    echo "expected failure for $file" >&2
    exit 1
  fi
  grep -E 'FAULT' /tmp/t81vm_example_err.txt >/dev/null
}

run_ok examples/runnable/arithmetic.t81vm
run_ok examples/runnable/policy_trace.t81vm
run_ok examples/runnable/arithmetic.tisc.json
run_fault examples/runnable/division_fault.t81vm

echo "examples-check: ok"
