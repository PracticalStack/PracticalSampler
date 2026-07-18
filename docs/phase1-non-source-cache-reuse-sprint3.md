# Phase 1 Sprint 3 Non-Source Cache Reuse

This note captures `S3.4-T4` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: preserve cache hits for unchanged sample sources when only zone mapping, gain, pan, or other non-source authoring edits change.

## What changed

- prepared-playback regression coverage now exercises broader non-source edits, not just a small gain/pan tweak
- the covered non-source edit set now includes:
  - root key
  - key range
  - velocity range
  - gain
  - pan
  - sample start
  - loop-enabled zone flag
- queue/worker regression coverage now proves the same warm-reuse behavior through `enqueuePreviewBuild(...)` and `processNextQueuedBuild(...)`

## Why this matters

This task turns the invalidation rule into a stronger behavioral guarantee:

- non-source authoring edits still change immutable snapshot and prepared zone content
- source-backed prepared assets stay warm
- cache keys, sample handles, stream handles, and ownership records remain unchanged
- decode work stays at zero for those edits

That is exactly the behavior we want before moving deeper into cache pressure and queue policy work.

## Verification

- `tests/src/Phase1PreparedPlaybackTests.cpp`
  - broader zone/mapping edits preserve warm prepared assets and cache-key identity
  - prepared zone content still reflects the edited mapping and mix values
- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - the queue/worker seam preserves full cache hits and zero decode work for the same non-source edit class
- guard regression remained green in:
  - `tests/src/Phase1DraftPlaybackFacadeTests.cpp`

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_draft_playback_facade_tests`
