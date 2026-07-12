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
- the reviewed drift window that CI is allowed to tolerate before the team refreshes the snapshot intentionally

## Checked-in snapshot rules

- `schemaName` and `schemaVersion` define the baseline file format
- `staticExpectations` should only change when the reference fixture contract changes intentionally
- `latestObserved` may change when the team decides to refresh the recorded baseline
- `driftPolicy` defines how much timing movement CI will tolerate before it asks for a reviewed snapshot refresh

## Sprint 1 intent

This format is the bridge between ad hoc timing output and the more formal benchmark history that later phases will need. It is deliberately small, text-based, and easy to diff.

Sprint 1 now also includes `drs_phase1_runtime_baseline_guard`, which runs after the generated report test and enforces two rules:

- static fields must match the checked-in snapshot exactly
- timing fields may drift only within the checked-in `driftPolicy` window

## Maintenance workflow

Use the fixture tool in one of two modes:

- `--verify` to confirm the checked-in project fixture, instrument fixture, and baseline snapshot are still consistent with the canonical serializer and live reference fixture
- `--write-reference-fixtures`, `--write-baseline`, or `--write-all` to intentionally refresh the checked-in files

When rewriting the baseline snapshot, the tool accepts `--captured-on YYYY-MM-DD`. If omitted, it uses the local machine date.

When the guard fails because timings moved outside the reviewed window, the workflow is to inspect the generated `phase1-runtime-baseline.json`, decide whether the drift is expected, and only then refresh the checked-in snapshot intentionally.
