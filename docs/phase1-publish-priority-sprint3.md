# Phase 1 Sprint 3 Publish Priority

This note captures `S3.5-T2` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: keep Publish above Preview when both lanes are contending for the same worker queue budget.

## What changed

- `PreparedPlaybackService` now treats queue admission and queue selection with the same priority rule.
- if the queue budget is already full and a Publish request arrives, the service may displace a queued lower-priority Preview request instead of rejecting Publish outright.
- lower-priority Preview requests still cannot displace queued Publish work when the queue is full.
- the worker regression now covers both sides of the rule:
  - queued Preview is displaced so Publish can enter a full queue
  - queued Publish is preserved when a Preview request arrives against the same full queue

## Why this matters

Before this slice, Publish already won once both jobs were sitting in the queue, but it could still lose earlier during admission if Preview had already consumed the available queued-worker budget.

That left a policy gap:

- execution priority said Publish matters more
- queue admission still behaved first-come-first-served under saturation

This change makes the priority rule consistent under load, which is what Sprint 3 needs before adding richer cancellation reasons and mixed-lane churn coverage.

## Verification

- `engine_adapter/src/PreparedPlayback.cpp`
  - priority-aware queue admission displacement
- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - full-queue Publish-over-Preview admission coverage
  - lower-priority Preview rejection against queued Publish

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_draft_playback_facade_tests`
