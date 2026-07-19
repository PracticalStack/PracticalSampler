# Phase 1 Sprint 3 Worker Status Hardening

This note captures Sprint 3 task `S3.7-T2` from section 6.1 of `engineering-plan.html`: eliminate public worker-state data races by making worker-status and queue inspection thread-safe.

## What changed

- `PreparedPlaybackService::getWorkerStatus()` now returns a `PreparedPlaybackWorkerStatus` snapshot by value while holding `workerMutex`.
- `PreparedPlaybackService::hasPendingQueuedBuilds()` now acquires `workerMutex` before inspecting the queued-job list.
- `EngineFacade::getPreparedPlaybackWorkerStatus()` now forwards the copied snapshot contract instead of exposing a reference-backed view.
- `EngineFacade::waitForPreparedPlaybackIdle(...)` now captures one local worker snapshot per poll iteration instead of reading unlocked public state repeatedly.

## Why this matters

- background-worker code mutates queue and worker-state fields concurrently with diagnostics, facade, and test readers
- returning a reference to shared mutable state was safe only by convention, not by contract
- the new copied-snapshot boundary is explicit, cheap for this status payload, and safe for Sprint 4 to depend on

## Verification

Validated on Sunday, July 19, 2026 with:

- `cmake --build --preset build-debug --target drs_phase1_prepared_playback_worker_tests drs_phase1_draft_playback_facade_tests drs_phase1_diagnostics_tests`
- `ctest --preset test-debug -R "drs.phase1.prepared_playback_worker|drs.phase1.draft_playback_facade|drs.phase1.diagnostics" --output-on-failure`
