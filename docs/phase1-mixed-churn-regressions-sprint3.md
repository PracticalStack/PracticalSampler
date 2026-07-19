# Phase 1 Sprint 3 Mixed Churn Regressions

This note captures `S3.5-T5` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: broaden the regression matrix so rapid Preview churn and mixed Preview/Publish churn both prove there are no orphaned jobs and no stale completions left applied after supersede.

## What changed

- worker coverage now queues multiple Preview and Publish revisions before the background worker can start, then proves only the newest Preview and newest Publish complete.
- that worker regression also proves there are no orphaned completed results left behind after the first completion drain.
- facade coverage now runs a rapid mixed Preview/Publish churn sequence across newer draft revisions and waits for full prepared-playback idle.
- the mixed facade regression then asserts the final state is the newest draft on both lanes, with no pending contract requests, no queued worker jobs, and matching Preview/Publish digests.

## Why this matters

Earlier Sprint 3.5 slices proved the local queue mechanics:

- Preview supersedes Preview
- Publish outranks Preview under pressure
- queue reasons and burst bounds are visible

What remained was a broader proof that those rules still compose correctly when authoring churn mixes both lanes across multiple draft revisions.

This slice closes that gap by checking the two failure modes we most want to avoid:

- orphaned worker completions that never get drained
- stale Preview or Publish completions that leave the facade settled on older content

## Verification

- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - deterministic mixed-lane queued supersede coverage before worker start
  - orphan-free completion drain coverage
- `tests/src/Phase1DraftPlaybackFacadeTests.cpp`
  - mixed Preview/Publish churn settle coverage at the facade boundary

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_diagnostics_tests`
- `drs_phase1_draft_playback_facade_tests`
