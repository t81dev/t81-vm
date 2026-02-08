#!/usr/bin/env python3
"""Deterministic VM throughput smoke/regression check."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import statistics
import subprocess
import tempfile
import time


STATE_HASH_RE = re.compile(r"^STATE_HASH\s+(0x[0-9a-fA-F]+)\s*$", re.MULTILINE)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--vm-bin",
        default="build/t81vm",
        help="Path to t81vm binary",
    )
    parser.add_argument(
        "--baseline",
        default="docs/benchmarks/vm-perf-baseline.json",
        help="Baseline JSON file",
    )
    parser.add_argument(
        "--report-out",
        default="build/perf/runtime-bench-report.json",
        help="Output report path",
    )
    return parser.parse_args()


def render_program(iterations: int) -> str:
    return "\n".join(
        [
            f"LoadImm 0 {iterations} 0",
            "LoadImm 1 1 0",
            "Add 2 2 1",
            "Dec 0 0 0",
            "JumpIfNotZero 2 0 0",
            "Halt 0 0 0",
            "",
        ]
    )


def run_vm(vm_bin: str, program_path: str, max_steps: int) -> tuple[float, str]:
    started = time.perf_counter()
    proc = subprocess.run(
        [vm_bin, "--snapshot", "--max-steps", str(max_steps), program_path],
        text=True,
        capture_output=True,
        check=False,
    )
    elapsed = time.perf_counter() - started
    if proc.returncode != 0:
        raise SystemExit(f"vm run failed: rc={proc.returncode}\n{proc.stderr.strip()}")
    match = STATE_HASH_RE.search(proc.stdout)
    if match is None:
        raise SystemExit("missing STATE_HASH in snapshot output")
    return elapsed, match.group(1).lower()


def main() -> int:
    args = parse_args()
    baseline_path = pathlib.Path(args.baseline)
    baseline = json.loads(baseline_path.read_text(encoding="utf-8"))

    iterations = int(baseline["iterations"])
    expected_steps = int(baseline["expected_steps"])
    max_steps = int(baseline["max_steps"])
    warmup_runs = int(baseline.get("warmup_runs", 1))
    measure_runs = int(baseline.get("measure_runs", 5))
    min_ips = float(baseline["min_instructions_per_sec"])
    expected_hash = str(baseline.get("expected_state_hash", "")).strip().lower()
    ref_ips = float(baseline.get("reference_median_instructions_per_sec", 0.0))
    max_reg_ratio = float(baseline.get("max_regression_ratio", 0.0))

    with tempfile.TemporaryDirectory(prefix="t81-vm-perf-") as td:
        program = pathlib.Path(td) / "runtime_bench.t81"
        program.write_text(render_program(iterations), encoding="utf-8")

        for _ in range(warmup_runs):
            run_vm(args.vm_bin, str(program), max_steps)

        timings: list[float] = []
        hashes: list[str] = []
        for _ in range(measure_runs):
            elapsed_s, state_hash = run_vm(args.vm_bin, str(program), max_steps)
            timings.append(elapsed_s)
            hashes.append(state_hash)

    unique_hashes = sorted(set(hashes))
    if len(unique_hashes) != 1:
        raise SystemExit(f"non-deterministic state hash in perf runs: {unique_hashes}")
    if expected_hash and unique_hashes[0] != expected_hash:
        raise SystemExit(
            f"unexpected state hash for benchmark workload: got={unique_hashes[0]} expected={expected_hash}"
        )

    median_s = statistics.median(timings)
    median_ips = expected_steps / median_s
    min_observed_ips = min(expected_steps / t for t in timings)

    if median_ips < min_ips:
        raise SystemExit(
            f"perf regression: median ips {median_ips:.2f} below baseline floor {min_ips:.2f}"
        )
    if ref_ips > 0.0 and max_reg_ratio > 0.0:
        floor_from_ref = ref_ips * max_reg_ratio
        if median_ips < floor_from_ref:
            raise SystemExit(
                "perf regression: median ips "
                f"{median_ips:.2f} below regression floor {floor_from_ref:.2f} "
                f"(ref={ref_ips:.2f}, ratio={max_reg_ratio:.2f})"
            )

    report = {
        "suite": baseline["suite"],
        "profile_id": baseline["profile_id"],
        "vm_bin": args.vm_bin,
        "iterations": iterations,
        "expected_steps": expected_steps,
        "max_steps": max_steps,
        "warmup_runs": warmup_runs,
        "measure_runs": measure_runs,
        "timings_seconds": timings,
        "state_hash": unique_hashes[0],
        "median_seconds": median_s,
        "median_instructions_per_sec": median_ips,
        "min_observed_instructions_per_sec": min_observed_ips,
        "baseline_floor_instructions_per_sec": min_ips,
    }
    out_path = pathlib.Path(args.report_out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        "perf-check: ok "
        f"(median_ips={median_ips:.2f}, min_ips={min_observed_ips:.2f}, hash={unique_hashes[0]})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
