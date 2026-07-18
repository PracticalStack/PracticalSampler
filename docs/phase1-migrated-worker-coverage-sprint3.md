# Phase 1 Sprint 3 Migrated Worker Coverage

This note captures `S3.3-T5` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: prove that imported authoring content on a migrated Phase 1 draft prepares correctly through the queue-driven worker path, not just through direct service calls or facade orchestration.

## What changed

- `Phase1PreparedPlaybackWorkerTests.cpp` now covers a migrated Phase 1 project after imported authoring content is appended.
- the worker regression now proves:
  - migrated imported content builds a valid preview snapshot
  - `enqueuePreviewBuild(...)` plus `processNextQueuedBuild(...)` prepares that content successfully
  - the first migrated preview cold-misses and decodes the imported content through the worker seam
  - the matching migrated publish build reuses the warmed prepared handles
  - preview and publish preserve the same immutable snapshot digest and prepared digest for equivalent migrated content

## Why this matters

Before this slice, Sprint 3 already had:

- direct prepared-playback coverage for migrated imported content
- facade-level migrated preview and publish coverage

What was still missing was the explicit queue/worker seam in the middle. This task closes that gap so the migrated path is now proven at all three relevant levels.

## Verification

- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - imported migrated preview prepares successfully on the preview lane
  - imported migrated publish prepares successfully on the publish lane
  - cold preview decodes through the worker seam
  - warm publish reuses the prepared cache without re-decoding
- guard regressions remained green in:
  - `tests/src/Phase1PreparedPlaybackTests.cpp`
  - `tests/src/Phase1DraftPlaybackFacadeTests.cpp`

Validated with:

- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_prepared_playback_tests`
- `drs_phase1_draft_playback_facade_tests`
