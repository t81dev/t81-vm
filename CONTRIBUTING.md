# Contributing to t81-vm

## Principles

- Determinism over convenience.
- Explicit contracts over implicit behavior.
- Reproducibility before optimization.

## Workflow

1. Open an issue describing the behavior change.
2. Link proposed change to `SPEC.md` section(s).
3. Add/update tests in `tests/` for normal and trap paths.
4. Include deterministic replay evidence in pull request notes.

## Pull Request Requirements

- Clear statement of behavioral impact.
- Spec alignment notes.
- Conformance test additions when behavior changes.
- No undocumented trap code additions.

## Commit Guidance

- Keep commits logically scoped.
- Use imperative subjects.
- Separate refactors from behavior changes.

## Review Criteria

- Deterministic output stability.
- Safety boundary preservation.
- Compatibility with declared `spec_version`.
