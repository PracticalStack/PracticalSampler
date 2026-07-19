# Phase 1 Sprint 3 Burst Bounds

This note captures `S3.5-T4` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: make worker concurrency and queue length explicit, bounded, and verifiable so bursty authoring edits degrade in a predictable way.

## What changed

- `PreparedPlaybackWorkerStatus` now exposes the configured queue budget and in-flight worker budget directly.
- `PreparedPlaybackService` refreshes those configured bounds into worker status alongside the observed pending and in-flight counts.
- `EngineDiagnosticsSnapshot`, `EnginePerformanceSnapshot`, and shell-facing status detail now expose those bounds so the shell can explain not only current backlog, but also the configured limits it is respecting.
- background-worker burst coverage now proves:
  - the queue never grows beyond its configured queued-work budget
  - in-flight worker activity never exceeds the configured single-worker concurrency budget
  - once a burst settles, only the surviving highest-priority bounded item completes

## Why this matters

Before this slice, the system already behaved as a single-worker queue with a capped pending list, but those constraints were mostly implicit in implementation details.

That left two review problems:

- shell diagnostics could report backlog without telling you what the configured limit actually was
- burst handling relied on reading the worker code instead of an explicit contract and regression

Making the limits first-class and testable reduces ambiguity when Preview and Publish churn arrives faster than preparation can finish.

## Verification

- `engine_adapter/include/drs/engine/PreparedPlayback.h`
  - explicit configured queue and in-flight worker bounds in status
- `engine_adapter/src/PreparedPlayback.cpp`
  - worker status refresh includes configured bounds
- `engine_adapter/src/EngineFacade.cpp`
  - shell-facing diagnostics and performance snapshots expose bounded-worker budgets
- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - background-worker burst saturation coverage
- `tests/src/Phase1DiagnosticsTests.cpp`
  - default diagnostics contract coverage
- `tests/src/Phase1DraftPlaybackFacadeTests.cpp`
  - snapshot-consistency coverage for bounded-worker budgets

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_diagnostics_tests`
- `drs_phase1_draft_playback_facade_tests`
