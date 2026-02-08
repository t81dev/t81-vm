# Mode Parity Evidence Policy

This policy defines retention and publication rules for execution-mode parity evidence artifacts.

## Artifact Contract

- Canonical artifact path: `build/mode-parity/parity-evidence.json`
- Schema: `parity-evidence-v1`
- Generator: `scripts/check-mode-parity.sh`
- Contract metadata source: `docs/contracts/vm-compatibility.json` (`execution_mode_parity_evidence`)

## Retention

- CI artifacts (`vm-mode-parity-evidence`, `vm-mode-parity-evidence-ecosystem`) are retained using the repository's default Actions artifact retention window.
- Release checkpoints must include at least one workflow URL proving parity artifact publication for the promoted runtime contract baseline.
- `t81-roadmap/MIGRATION_CHECKPOINTS.md` is the durable index of evidence links for promoted baselines.

## Publication Rules

1. Any runtime contract promotion must include parity evidence artifact publication in `ci` and `ecosystem-contract` workflows.
2. The artifact must report `overall_ok: true` for all vectors listed in `execution_mode_parity_evidence.canonical_vectors`.
3. Any change to parity signals, canonical vectors, schema, or artifact path requires:
   - contract metadata update,
   - validator update (`scripts/check-vm-contract.py`),
   - release note entry in `CHANGELOG.md`.
