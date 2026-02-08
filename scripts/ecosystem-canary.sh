#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VM_DIR="${VM_DIR:-${ROOT}/t81-vm}"
LANG_DIR="${LANG_DIR:-${ROOT}/t81-lang}"
PY_DIR="${PY_DIR:-${ROOT}/t81-python}"
OUT_DIR="${OUT_DIR:-${VM_DIR}/build/canary}"

mkdir -p "${OUT_DIR}"

echo "[1/5] Build VM artifacts"
make -C "${VM_DIR}" build-check

echo "[2/5] Emit deterministic canary bytecode from t81-lang"
python3 "${LANG_DIR}/scripts/emit-canary-bytecode.py" "${OUT_DIR}/canary.tisc.json"

echo "[3/5] Execute canary in t81-vm CLI"
"${VM_DIR}/build/t81vm" --trace --snapshot "${OUT_DIR}/canary.tisc.json" > "${OUT_DIR}/cli.out"
if ! grep -q "STATE_HASH " "${OUT_DIR}/cli.out"; then
  echo "missing STATE_HASH in CLI output" >&2
  exit 1
fi

echo "[4/5] Verify t81-lang compatibility gate"
T81_VM_DIR="${VM_DIR}" python3 "${LANG_DIR}/scripts/check-vm-compat.py"

echo "[5/5] Execute canary via t81-python VM bridge"
VM_LIB=""
if [[ -f "${VM_DIR}/build/libt81vm_capi.dylib" ]]; then
  VM_LIB="${VM_DIR}/build/libt81vm_capi.dylib"
elif [[ -f "${VM_DIR}/build/libt81vm_capi.so" ]]; then
  VM_LIB="${VM_DIR}/build/libt81vm_capi.so"
else
  echo "missing libt81vm_capi shared library" >&2
  exit 1
fi

T81_VM_LIB="${VM_LIB}" \
PYTHONPATH="${PY_DIR}/src" \
python3 "${PY_DIR}/scripts/run_vm_canary.py" "${OUT_DIR}/canary.tisc.json"

echo "ecosystem canary: ok"
