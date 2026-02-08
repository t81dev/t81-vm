#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VM_BIN="${ROOT}/build/t81vm"
CONTRACT_FILE="${ROOT}/docs/contracts/vm-compatibility.json"
OUT_DIR="${ROOT}/build/mode-parity"
JSON_OUT="${PARITY_EVIDENCE_OUT:-${OUT_DIR}/parity-evidence.json}"
NDJSON="${OUT_DIR}/parity-evidence.ndjson"

mkdir -p "${OUT_DIR}"

if [[ ! -x "${VM_BIN}" ]]; then
  echo "missing VM binary: ${VM_BIN}" >&2
  exit 1
fi

if [[ ! -f "${CONTRACT_FILE}" ]]; then
  echo "missing contract file: ${CONTRACT_FILE}" >&2
  exit 1
fi

mapfile -t PARITY_VECTORS < <(python3 - "${CONTRACT_FILE}" <<'PY'
import json
import sys
from pathlib import Path

contract = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
evidence = contract.get("execution_mode_parity_evidence", {})
for vector in evidence.get("canonical_vectors", []):
    if str(vector).strip():
        print(str(vector).strip())
PY
)

mapfile -t REQUIRED_SIGNALS < <(python3 - "${CONTRACT_FILE}" <<'PY'
import json
import sys
from pathlib import Path

contract = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
evidence = contract.get("execution_mode_parity_evidence", {})
for signal in evidence.get("required_equal_signals", []):
    if str(signal).strip():
        print(str(signal).strip())
PY
)

if [[ "${#PARITY_VECTORS[@]}" -eq 0 ]]; then
  echo "no canonical parity vectors found in contract evidence metadata" >&2
  exit 1
fi

if [[ "${#REQUIRED_SIGNALS[@]}" -eq 0 ]]; then
  echo "no required parity signals found in contract evidence metadata" >&2
  exit 1
fi

: > "${NDJSON}"
overall_ok=1

extract_trap_class() {
  local file="$1"
  local line
  line="$(grep '^FAULT ' "${file}" | head -n1 || true)"
  if [[ -z "${line}" ]]; then
    echo ""
    return 0
  fi
  awk '{print $2}' <<<"${line}"
}

extract_trap_payload() {
  local file="$1"
  grep '^TRAP_PAYLOAD ' "${file}" | head -n1 || true
}

for vector in "${PARITY_VECTORS[@]}"; do
  vector_path="${ROOT}/${vector}"
  if [[ ! -f "${vector_path}" ]]; then
    echo "missing parity vector: ${vector_path}" >&2
    exit 1
  fi

  vector_id="$(tr '/.' '__' <<<"${vector}")"
  int_out="${OUT_DIR}/${vector_id}.interpreter.out"
  int_err="${OUT_DIR}/${vector_id}.interpreter.err"
  acc_out="${OUT_DIR}/${vector_id}.accelerated_preview.out"
  acc_err="${OUT_DIR}/${vector_id}.accelerated_preview.err"
  int_all="${OUT_DIR}/${vector_id}.interpreter.all"
  acc_all="${OUT_DIR}/${vector_id}.accelerated_preview.all"

  set +e
  "${VM_BIN}" --snapshot --mode interpreter "${vector_path}" > "${int_out}" 2> "${int_err}"
  int_rc=$?
  "${VM_BIN}" --snapshot --mode accelerated-preview "${vector_path}" > "${acc_out}" 2> "${acc_err}"
  acc_rc=$?
  set -e

  cat "${int_out}" "${int_err}" > "${int_all}"
  cat "${acc_out}" "${acc_err}" > "${acc_all}"

  mode_marker_ok=true
  if ! grep -q "^MODE accelerated-preview" "${acc_err}"; then
    mode_marker_ok=false
    overall_ok=0
  fi

  state_hash_int="$(grep '^STATE_HASH ' "${int_out}" | tail -n1 || true)"
  state_hash_acc="$(grep '^STATE_HASH ' "${acc_out}" | tail -n1 || true)"
  trap_class_int="$(extract_trap_class "${int_all}")"
  trap_class_acc="$(extract_trap_class "${acc_all}")"
  trap_payload_int="$(extract_trap_payload "${int_all}")"
  trap_payload_acc="$(extract_trap_payload "${acc_all}")"

  vector_ok=1
  mismatch_reasons=()

  if [[ "${int_rc}" -ne "${acc_rc}" ]]; then
    mismatch_reasons+=("exit_code")
    vector_ok=0
  fi

  for signal in "${REQUIRED_SIGNALS[@]}"; do
    case "${signal}" in
      STATE_HASH)
        if [[ "${state_hash_int}" != "${state_hash_acc}" ]]; then
          mismatch_reasons+=("STATE_HASH")
          vector_ok=0
        fi
        ;;
      TRAP_CLASS)
        if [[ "${trap_class_int}" != "${trap_class_acc}" ]]; then
          mismatch_reasons+=("TRAP_CLASS")
          vector_ok=0
        fi
        ;;
      TRAP_PAYLOAD)
        if [[ "${trap_payload_int}" != "${trap_payload_acc}" ]]; then
          mismatch_reasons+=("TRAP_PAYLOAD")
          vector_ok=0
        fi
        ;;
      *)
        mismatch_reasons+=("unknown_signal:${signal}")
        vector_ok=0
        ;;
    esac
  done

  if [[ "${mode_marker_ok}" != "true" ]]; then
    mismatch_reasons+=("mode_marker")
    vector_ok=0
  fi

  if [[ "${vector_ok}" -ne 1 ]]; then
    overall_ok=0
    echo "mode parity failure: ${vector}" >&2
    echo "mismatches: ${mismatch_reasons[*]}" >&2
  fi

  python3 - "${NDJSON}" "${vector}" "${int_rc}" "${acc_rc}" "${state_hash_int}" "${state_hash_acc}" "${trap_class_int}" "${trap_class_acc}" "${trap_payload_int}" "${trap_payload_acc}" "${mode_marker_ok}" "${vector_ok}" "$(IFS=,; echo "${mismatch_reasons[*]:-}")" <<'PY'
import json
import sys
from pathlib import Path

ndjson_path = Path(sys.argv[1])
record = {
    "vector": sys.argv[2],
    "interpreter_exit_code": int(sys.argv[3]),
    "accelerated_preview_exit_code": int(sys.argv[4]),
    "state_hash": {"interpreter": sys.argv[5], "accelerated_preview": sys.argv[6]},
    "trap_class": {"interpreter": sys.argv[7], "accelerated_preview": sys.argv[8]},
    "trap_payload": {"interpreter": sys.argv[9], "accelerated_preview": sys.argv[10]},
    "mode_marker_present": sys.argv[11].lower() == "true",
    "parity_ok": sys.argv[12] == "1",
    "mismatches": [x for x in sys.argv[13].split(",") if x],
}
with ndjson_path.open("a", encoding="utf-8") as f:
    f.write(json.dumps(record, sort_keys=True))
    f.write("\n")
PY
done

python3 - "${NDJSON}" "${JSON_OUT}" "${CONTRACT_FILE}" <<'PY'
import json
import sys
from pathlib import Path

records_path = Path(sys.argv[1])
out_path = Path(sys.argv[2])
contract_path = Path(sys.argv[3])
contract = json.loads(contract_path.read_text(encoding="utf-8"))
evidence = contract.get("execution_mode_parity_evidence", {})
records = []
for line in records_path.read_text(encoding="utf-8").splitlines():
    if line.strip():
        records.append(json.loads(line))
payload = {
    "schema_version": "parity-evidence-v1",
    "contract_version": contract.get("contract_version", ""),
    "required_equal_signals": evidence.get("required_equal_signals", []),
    "canonical_vectors": evidence.get("canonical_vectors", []),
    "overall_ok": all(record.get("parity_ok") for record in records),
    "results": records,
}
out_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
print(f"wrote {out_path}")
PY

if [[ "${overall_ok}" -ne 1 ]]; then
  exit 1
fi

echo "mode parity: ok (${JSON_OUT})"
