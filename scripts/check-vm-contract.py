#!/usr/bin/env python3
"""Validate the VM runtime compatibility contract artifact."""

from __future__ import annotations

import json
import os
from pathlib import Path


REQUIRED_TOP_LEVEL = {
    "contract_version",
    "runtime_owner",
    "accepted_program_formats",
    "state_hash",
    "trace_contract",
    "execution_modes",
    "execution_mode_parity_evidence",
    "compatibility_governance",
    "trap_payload_contract",
    "trap_registry",
    "supported_opcodes",
    "host_abi",
}

REQUIRED_FORMATS = {"TextV1", "TiscJsonV1"}
REQUIRED_TRAPS = {"DecodeFault", "TypeFault", "DivisionFault", "TrapInstruction"}
REQUIRED_EXECUTION_MODES = {"interpreter", "accelerated-preview"}
REQUIRED_PARITY_SIGNALS = {"STATE_HASH", "TRAP_CLASS", "TRAP_PAYLOAD"}
REQUIRED_PARITY_VECTORS = {
    "tests/harness/test_vectors/arithmetic.t81",
    "tests/harness/test_vectors/faults.t81",
    "tests/harness/test_vectors/tensor_fault_chain.t81",
}
REQUIRED_OPCODES = {
    "Nop",
    "Halt",
    "LoadImm",
    "Load",
    "Store",
    "Add",
    "Sub",
    "Mul",
    "Div",
    "Mod",
    "Jump",
    "JumpIfZero",
    "JumpIfNotZero",
    "Cmp",
    "Trap",
    "I2F",
    "F2I",
    "I2Frac",
    "Frac2I",
    "FAdd",
    "FSub",
    "FMul",
    "FDiv",
    "FracAdd",
    "FracSub",
    "FracMul",
    "FracDiv",
    "Less",
    "LessEqual",
    "Greater",
    "GreaterEqual",
    "Equal",
    "NotEqual",
    "TNot",
    "TAnd",
    "TOr",
    "TXor",
    "AxRead",
    "AxSet",
    "AxVerify",
    "TVecAdd",
    "TMatMul",
    "TTenDot",
    "TVecMul",
    "TTranspose",
    "TExp",
    "TSqrt",
    "TSiLU",
    "TSoftmax",
    "TRMSNorm",
    "TRoPE",
    "ChkShape",
    "WeightsLoad",
    "SetF",
    "MakeOptionSome",
    "MakeOptionNone",
    "MakeResultOk",
    "MakeResultErr",
    "OptionIsSome",
    "OptionUnwrap",
    "ResultIsOk",
    "ResultUnwrapOk",
    "ResultUnwrapErr",
    "MakeEnumVariant",
    "MakeEnumVariantPayload",
    "EnumIsVariant",
    "EnumUnwrapPayload",
}


def main() -> None:
    root = Path(__file__).resolve().parent.parent
    contract_path = root / "docs/contracts/vm-compatibility.json"
    contract = json.loads(contract_path.read_text(encoding="utf-8"))

    missing_keys = sorted(REQUIRED_TOP_LEVEL - set(contract))
    if missing_keys:
        raise SystemExit(f"Missing top-level keys in contract: {', '.join(missing_keys)}")

    if contract.get("runtime_owner") != "t81-vm":
        raise SystemExit("runtime_owner must be 't81-vm'")

    contract_version = str(contract.get("contract_version", "")).strip()
    if not contract_version:
        raise SystemExit("contract_version must be non-empty")

    formats = {entry.get("name") for entry in contract.get("accepted_program_formats", [])}
    missing_formats = sorted(REQUIRED_FORMATS - formats)
    if missing_formats:
        raise SystemExit(f"Missing accepted program formats: {', '.join(missing_formats)}")

    traps = set(contract.get("trap_registry", []))
    missing_traps = sorted(REQUIRED_TRAPS - traps)
    if missing_traps:
        raise SystemExit(f"Missing required traps: {', '.join(missing_traps)}")

    opcodes = set(contract.get("supported_opcodes", []))
    missing_opcodes = sorted(REQUIRED_OPCODES - opcodes)
    if missing_opcodes:
        raise SystemExit(f"Missing required opcodes: {', '.join(missing_opcodes)}")

    host_abi = contract.get("host_abi", {})
    for field in ("name", "header", "library", "version"):
        if not str(host_abi.get(field, "")).strip():
            raise SystemExit(f"host_abi.{field} must be non-empty")

    trace_contract = contract.get("trace_contract", {})
    if not str(trace_contract.get("format_version", "")).strip():
        raise SystemExit("trace_contract.format_version must be non-empty")

    execution_modes = contract.get("execution_modes", [])
    mode_names = {entry.get("name") for entry in execution_modes}
    missing_modes = sorted(REQUIRED_EXECUTION_MODES - mode_names)
    if missing_modes:
        raise SystemExit(f"Missing required execution modes: {', '.join(missing_modes)}")

    parity_evidence = contract.get("execution_mode_parity_evidence", {})
    parity_schema_version = str(parity_evidence.get("schema_version", "")).strip()
    if parity_schema_version != "parity-evidence-v1":
        raise SystemExit(
            "execution_mode_parity_evidence.schema_version must be 'parity-evidence-v1'"
        )

    parity_artifact_path = str(parity_evidence.get("artifact_path", "")).strip()
    if not parity_artifact_path:
        raise SystemExit("execution_mode_parity_evidence.artifact_path must be non-empty")

    parity_generator = str(parity_evidence.get("generator", "")).strip()
    if not parity_generator:
        raise SystemExit("execution_mode_parity_evidence.generator must be non-empty")

    baseline_mode = str(parity_evidence.get("baseline_mode", "")).strip()
    if baseline_mode != "interpreter":
        raise SystemExit("execution_mode_parity_evidence.baseline_mode must be 'interpreter'")

    candidate_modes = {
        str(mode).strip()
        for mode in parity_evidence.get("candidate_modes", [])
        if str(mode).strip()
    }
    if "accelerated-preview" not in candidate_modes:
        raise SystemExit(
            "execution_mode_parity_evidence.candidate_modes must include 'accelerated-preview'"
        )

    parity_signals = {
        str(signal).strip()
        for signal in parity_evidence.get("required_equal_signals", [])
        if str(signal).strip()
    }
    missing_signals = sorted(REQUIRED_PARITY_SIGNALS - parity_signals)
    if missing_signals:
        raise SystemExit(
            "execution_mode_parity_evidence missing required_equal_signals: "
            + ", ".join(missing_signals)
        )

    parity_vectors = {
        str(vector).strip()
        for vector in parity_evidence.get("canonical_vectors", [])
        if str(vector).strip()
    }
    missing_vectors = sorted(REQUIRED_PARITY_VECTORS - parity_vectors)
    if missing_vectors:
        raise SystemExit(
            "execution_mode_parity_evidence missing canonical_vectors: "
            + ", ".join(missing_vectors)
        )
    for vector in sorted(parity_vectors):
        if not (root / vector).exists():
            raise SystemExit(f"execution_mode_parity_evidence vector not found: {vector}")

    require_artifact = os.environ.get("REQUIRE_PARITY_ARTIFACT", "").strip() == "1"
    if require_artifact:
        artifact = root / parity_artifact_path
        if not artifact.exists():
            raise SystemExit(
                "execution_mode_parity_evidence artifact missing: "
                f"{parity_artifact_path}; run {parity_generator}"
            )
        evidence = json.loads(artifact.read_text(encoding="utf-8"))
        if evidence.get("schema_version") != parity_schema_version:
            raise SystemExit("parity evidence artifact schema_version mismatch")
        if evidence.get("contract_version") != contract_version:
            raise SystemExit("parity evidence artifact contract_version mismatch")

    compatibility_governance = contract.get("compatibility_governance", {})
    marker_contract_path = str(compatibility_governance.get("marker_contract_path", "")).strip()
    if marker_contract_path != "contracts/runtime-contract.json":
        raise SystemExit(
            "compatibility_governance.marker_contract_path must be "
            "'contracts/runtime-contract.json'"
        )

    matrix_workflow = str(
        compatibility_governance.get("cross_repo_matrix_workflow", "")
    ).strip()
    if matrix_workflow != ".github/workflows/ecosystem-compat-matrix.yml":
        raise SystemExit(
            "compatibility_governance.cross_repo_matrix_workflow must be "
            "'.github/workflows/ecosystem-compat-matrix.yml'"
        )

    deterministic_fixture = str(compatibility_governance.get("deterministic_fixture", "")).strip()
    if deterministic_fixture != "t81-examples/scripts/run-runtime-v0.5-e2e.sh":
        raise SystemExit(
            "compatibility_governance.deterministic_fixture must be "
            "'t81-examples/scripts/run-runtime-v0.5-e2e.sh'"
        )

    required_release_artifact = str(
        compatibility_governance.get("required_release_artifact", "")
    ).strip()
    if required_release_artifact != "runtime-v0.5-e2e-evidence":
        raise SystemExit(
            "compatibility_governance.required_release_artifact must be "
            "'runtime-v0.5-e2e-evidence'"
        )

    trap_payload_contract = contract.get("trap_payload_contract", {})
    if not str(trap_payload_contract.get("format_version", "")).strip():
        raise SystemExit("trap_payload_contract.format_version must be non-empty")
    if "TRAP_PAYLOAD" not in str(trap_payload_contract.get("summary_line", "")):
        raise SystemExit("trap_payload_contract.summary_line must describe TRAP_PAYLOAD output")

    state_hash = contract.get("state_hash", {})
    if not str(state_hash.get("name", "")).strip():
        raise SystemExit("state_hash.name must be non-empty")

    print("vm contract validation: ok")


if __name__ == "__main__":
    main()
