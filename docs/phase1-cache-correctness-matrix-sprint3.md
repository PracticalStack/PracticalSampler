# Phase 1 Sprint 3 Cache Correctness Matrix

This note captures `S3.4-T5` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: add a focused regression matrix for the three cache-correctness edit classes we keep relying on during Sprint 3.

## What changed

- `Phase1PreparedPlaybackTests.cpp` now names and verifies a compact cache-correctness matrix for:
  - replace-sample
  - relink-sample
  - zone-only edits
- the regression now asserts the expected prepared cache-key delta count for each case:
  - replace-sample: exactly one key changes
  - relink-sample: exactly one key changes
  - zone-only edits: zero keys change

## Why this matters

By this point in Sprint 3, the relevant behaviors already existed across several broader tests. What was missing was one clearly readable matrix that states the expected cache outcome for the main authoring edit classes.

That makes the cache contract easier to review:

- replace-sample should rebuild the affected asset
- relink-sample should rebuild the affected asset
- zone-only edits should keep every source-backed prepared asset warm

## Verification

- `tests/src/Phase1PreparedPlaybackTests.cpp`
  - replace-sample matrix case
  - relink-sample matrix case
  - zone-only matrix case
- guard regressions remained green in:
  - `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - `tests/src/Phase1DraftPlaybackFacadeTests.cpp`

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_draft_playback_facade_tests`
