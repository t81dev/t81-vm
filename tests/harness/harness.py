import hashlib
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent
VECTORS = ROOT / "test_vectors"
VM = Path("build/t81vm")


def run_vm(path: Path):
    result = subprocess.run(
        [str(VM), "--trace", "--snapshot", str(path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return result.returncode, result.stdout, result.stderr


def trace_hash(trace: str) -> str:
    return hashlib.sha256(trace.encode("utf-8")).hexdigest()


def extract_state_hash(stdout: str) -> str:
    for line in stdout.splitlines():
        if line.startswith("STATE_HASH "):
            return line.split(" ", 1)[1].strip()
    raise AssertionError("Missing STATE_HASH line")


def test_determinism():
    print("[1] Determinism")
    for program in sorted(VECTORS.glob("*.t81")):
        first = run_vm(program)
        second = run_vm(program)

        if first[0] != second[0]:
            raise AssertionError(f"Return code mismatch in {program.name}")
        if trace_hash(first[1]) != trace_hash(second[1]):
            raise AssertionError(f"Trace mismatch in {program.name}")
        if extract_state_hash(first[1]) != extract_state_hash(second[1]):
            raise AssertionError(f"State hash mismatch in {program.name}")

    print("✓ Determinism verified")


def test_fault_vectors():
    print("[2] Fault behavior")
    for name in ["faults.t81", "bounds_fault.t81"]:
        rc, _out, err = run_vm(VECTORS / name)
        if rc == 0:
            raise AssertionError(f"Expected fault for {name}")
        if "FAULT" not in err:
            raise AssertionError(f"Missing FAULT marker for {name}")

    print("✓ Fault behavior verified")


def main():
    import sys

    if len(sys.argv) != 2 or sys.argv[1] != "run":
        print("Usage: harness.py run")
        raise SystemExit(2)

    test_determinism()
    test_fault_vectors()


if __name__ == "__main__":
    main()
