# Phase 1 Sprint 3 Preview Coalescing

This note captures `S3.5-T1` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: make repeated Preview requests coalesce explicitly at the facade boundary so only the newest relevant draft completion remains eligible to apply.

## What changed

- `EngineFacade` now discards older pending Preview completions as soon as a newer Preview build is accepted by the prepared-playback service.
- queued Preview jobs were already superseded inside `PreparedPlaybackService`; this slice closes the gap for stale Preview completions that were already in flight.
- the rapid-preview facade regression now checks that:
  - revision `4` is the one that survives after repeated Preview churn
  - no Preview completion remains pending after the worker settles
  - the published revision remains unchanged while Preview churn settles

## Why this matters

Before this slice, the system still behaved correctly because the draft-playback contract rejected stale Preview completions by request id. That was safe, but the coalescing rule was implicit.

Making the facade drop obsolete Preview completions earlier does two useful things:

- it makes the newest-preview-wins rule visible in one place
- it prevents stale in-flight Preview results from remaining tracked longer than necessary

This keeps the Sprint 3 queue-control contract easier to reason about before we add mixed Preview/Publish priority and explicit cancellation reasons in later 3.5 tasks.

## Verification

- `engine_adapter/src/EngineFacade.cpp`
  - explicit pruning of superseded Preview pending completions
- `tests/src/Phase1DraftPlaybackFacadeTests.cpp`
  - rapid Preview churn regression with supersede and idle assertions

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_draft_playback_facade_tests`
