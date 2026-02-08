#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VM_BIN="${ROOT}/build/t81vm"
PROGRAM="${ROOT}/tests/harness/test_vectors/arithmetic.t81"
OUT_DIR="${ROOT}/build/mode-parity"

mkdir -p "${OUT_DIR}"

if [[ ! -x "${VM_BIN}" ]]; then
  echo "missing VM binary: ${VM_BIN}" >&2
  exit 1
fi

"${VM_BIN}" --snapshot --mode interpreter "${PROGRAM}" > "${OUT_DIR}/interpreter.out"
"${VM_BIN}" --snapshot --mode accelerated-preview "${PROGRAM}" > "${OUT_DIR}/accelerated_preview.out" 2> "${OUT_DIR}/accelerated_preview.err"

if ! grep -q "^MODE accelerated-preview" "${OUT_DIR}/accelerated_preview.err"; then
  echo "missing accelerated-preview mode marker" >&2
  exit 1
fi

grep '^STATE_HASH ' "${OUT_DIR}/interpreter.out" > "${OUT_DIR}/interpreter.hash"
grep '^STATE_HASH ' "${OUT_DIR}/accelerated_preview.out" > "${OUT_DIR}/accelerated_preview.hash"

if ! cmp -s "${OUT_DIR}/interpreter.hash" "${OUT_DIR}/accelerated_preview.hash"; then
  echo "mode parity failure: STATE_HASH mismatch" >&2
  diff -u "${OUT_DIR}/interpreter.hash" "${OUT_DIR}/accelerated_preview.hash" >&2 || true
  exit 1
fi

echo "mode parity: ok"
