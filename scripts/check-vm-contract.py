#!/usr/bin/env python3
"""Validate the VM runtime compatibility contract artifact."""

from __future__ import annotations

import json
from pathlib import Path


REQUIRED_TOP_LEVEL = {
    "contract_version",
    "runtime_owner",
    "accepted_program_formats",
    "state_hash",
    "trace_contract",
    "trap_registry",
    "supported_opcodes",
    "host_abi",
}

REQUIRED_FORMATS = {"TextV1", "TiscJsonV1"}
REQUIRED_TRAPS = {"DecodeFault", "TypeFault", "DivisionFault", "TrapInstruction"}
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

    state_hash = contract.get("state_hash", {})
    if not str(state_hash.get("name", "")).strip():
        raise SystemExit("state_hash.name must be non-empty")

    print("vm contract validation: ok")


if __name__ == "__main__":
    main()
