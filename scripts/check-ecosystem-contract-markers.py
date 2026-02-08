#!/usr/bin/env python3
"""Validate runtime contract marker alignment across ecosystem repos."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def load_json(path: Path) -> dict:
    if not path.exists():
        raise SystemExit(f"Missing JSON file: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--vm-dir", required=True, type=Path)
    parser.add_argument("--repo-dir", required=True, type=Path)
    parser.add_argument("--repo-name", required=True)
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    vm_contract = load_json(args.vm_dir / "docs/contracts/vm-compatibility.json")
    vm_marker = load_json(args.vm_dir / "contracts/runtime-contract.json")
    repo_marker = load_json(args.repo_dir / "contracts/runtime-contract.json")

    vm_contract_version = str(vm_contract.get("contract_version", "")).strip()
    vm_baseline_pin = str(vm_marker.get("vm_main_pin", "")).strip()
    if not vm_baseline_pin:
        raise SystemExit("t81-vm marker missing vm_main_pin")

    repo_contract_version = str(repo_marker.get("contract_version", "")).strip()
    if repo_contract_version != vm_contract_version:
        raise SystemExit(
            f"{args.repo_name}: contract_version mismatch "
            f"(repo={repo_contract_version!r}, vm={vm_contract_version!r})"
        )

    repo_tag = str(repo_marker.get("runtime_tag", "")).strip()
    vm_tag = str(vm_marker.get("runtime_tag", "")).strip()
    if repo_tag != vm_tag:
        raise SystemExit(
            f"{args.repo_name}: runtime_tag mismatch (repo={repo_tag!r}, vm={vm_tag!r})"
        )

    pinned = str(repo_marker.get("vm_main_pin", "")).strip()
    if not pinned:
        raise SystemExit(f"{args.repo_name}: vm_main_pin must be non-empty")
    if pinned != vm_baseline_pin:
        raise SystemExit(
            f"{args.repo_name}: vm_main_pin mismatch "
            f"(repo={pinned}, vm_baseline={vm_baseline_pin})"
        )

    print(f"{args.repo_name}: runtime contract marker ok")


if __name__ == "__main__":
    main()
