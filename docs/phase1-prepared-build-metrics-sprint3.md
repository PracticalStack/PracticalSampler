# Phase 1 Prepared Build Metrics Note

This note captures Sprint 3 task `S3.6-T3` from section 6.1 of `engineering-plan.html`: expose prepared-build timing and cache/decode metrics through shell-facing snapshots.

## What is now visible

- preview and published prepared build duration in microseconds
- preview and published decoded-byte totals
- preview and published prepared sample-data byte totals
- preview and published cache hit counts and miss counts
- preview and published derived prepared cache hit rates
- existing worker pressure fields remain visible beside these per-build metrics:
  - queued work
  - cancellations
  - failures
  - active ownership bytes
  - retired ownership bytes

## Wiring

- `DraftPlaybackPreparedRevision` now retains the prepared-build metrics needed after worker completion
- `EnginePerformanceSnapshot` and `EngineDiagnosticsSnapshot` now surface those metrics for both Preview and Publish
- shell detail now includes a `Prepared build metrics:` line so the contract is reviewable without debugger-only access

## Why this matters

- shell and diagnostics consumers can now distinguish cold versus warm prepared builds
- decode-heavy rebuilds are visible without dropping into worker-level tests
- cache effectiveness is reviewable as both raw counts and normalized hit rate

## Regression coverage

- `tests/src/Phase1DiagnosticsTests.cpp`
  - verifies prepared build duration, decoded bytes, sample-data bytes, and normalized hit rates are present
- `tests/src/Phase1DraftPlaybackFacadeTests.cpp`
  - verifies diagnostics/performance alignment and shell detail reporting for the new metrics
- `tests/src/Phase0SmokeTests.cpp`
  - verifies the shell detail includes the prepared build metrics line
