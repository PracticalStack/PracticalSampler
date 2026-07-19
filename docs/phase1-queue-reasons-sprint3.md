# Phase 1 Sprint 3 Queue Reasons

This note captures `S3.5-T3` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: preserve explicit queue cancellation and supersede reasons all the way into shell-facing diagnostics instead of collapsing them into a generic worker event string.

## What changed

- `PreparedPlaybackWorkerStatus` now carries:
  - last cancellation lane
  - last cancellation reason
  - last superseded lane
  - last supersede reason
- `PreparedPlaybackService` records those fields whenever queued work is canceled or displaced.
- `EngineDiagnosticsSnapshot`, `EnginePerformanceSnapshot`, and the shell-facing status detail now expose those queue-reason fields alongside the existing worker counters and last-event text.
- regression coverage now verifies:
  - worker-level same-lane preview supersede reasons
  - worker-level explicit preview cancellation reasons
  - shell-facing snapshot/detail propagation for the structured queue-reason fields

## Why this matters

By Sprint 3.5 we already knew how many jobs were canceled or superseded, but the shell still could not answer the more practical question:

- what exactly got canceled
- which lane was displaced
- why that displacement happened

Those details matter during real authoring churn because they distinguish:

- expected preview churn
- publish-priority displacement
- restart or project-close cancellation

Surfacing the reason separately from the worker’s generic last-event string makes the queue behavior easier to review and less ambiguous when multiple events happen close together.

## Verification

- `engine_adapter/include/drs/engine/PreparedPlayback.h`
  - structured worker-status queue-reason fields
- `engine_adapter/src/PreparedPlayback.cpp`
  - queue cancellation and supersede reason recording
- `engine_adapter/src/EngineFacade.cpp`
  - diagnostics, performance snapshot, and shell-detail propagation
- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - worker reason coverage
- `tests/src/Phase1DraftPlaybackFacadeTests.cpp`
  - shell-facing snapshot-contract coverage for queue-reason fields
- `tests/src/Phase1DiagnosticsTests.cpp`
  - default diagnostics contract coverage

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_diagnostics_tests`
- `drs_phase1_draft_playback_facade_tests`
