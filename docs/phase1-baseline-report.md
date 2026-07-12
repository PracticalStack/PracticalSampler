# Phase 1 Baseline Report Format

Sprint 1 now has two baseline-report artifacts:

- a generated runtime report produced by `drs_phase1_runtime_baseline_report`
- a checked-in reference snapshot at `content/runtime/phase1/baselines/tiny-open-instrument-baseline.json`
- a maintenance utility at `drs_phase1_runtime_fixture_tool` that can verify or intentionally rewrite the checked-in fixtures and baseline snapshot

## Why both exist

The generated report gives the current measurement for the active build.

The checked-in snapshot gives the repository a stable historical anchor for:

- report shape
- metric names
- static expectations tied to the reference fixture
- the first recorded observation for cold and warm manifest load time

## Checked-in snapshot rules

- `schemaName` and `schemaVersion` define the baseline file format
- `staticExpectations` should only change when the reference fixture contract changes intentionally
- `latestObserved` may change when the team decides to refresh the recorded baseline
- timing values are informative observations, not hard budgets yet

## Sprint 1 intent

This format is the bridge between ad hoc timing output and the more formal benchmark history that later phases will need. It is deliberately small, text-based, and easy to diff.

## Maintenance workflow

Use the fixture tool in one of two modes:

- `--verify` to confirm the checked-in project fixture, instrument fixture, and baseline snapshot are still consistent with the canonical serializer and live reference fixture
- `--write-reference-fixtures`, `--write-baseline`, or `--write-all` to intentionally refresh the checked-in files

When rewriting the baseline snapshot, the tool accepts `--captured-on YYYY-MM-DD`. If omitted, it uses the local machine date.
