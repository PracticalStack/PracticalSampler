# Phase 1 Sprint 3 Targeted Prepared Invalidation

This note captures `S3.4-T3` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: invalidate exactly the prepared assets affected by source-path, checksum, loop-relevant decode-policy, or compiler-version changes.

## What changed

- the prepared decode-policy fingerprint now includes loop-relevant fields:
  - `loopRangePresent`
  - `loopStartFrame`
  - `loopEndFrame`
- prepared-playback regression coverage now proves targeted invalidation for four change classes:
  - source path change
  - source checksum change with stable path
  - loop-relevant decode-policy change with stable path and checksum
  - compiler/version salt change

## Why this matters

Sprint 3 now distinguishes:

- identity/content changes that should rebuild only the affected prepared asset
- policy changes that should also rebuild only the affected prepared asset
- unrelated assets that should stay warm across those edits

This keeps invalidation precise instead of globally cold-starting prepared playback whenever one input changes.

## Verification

- `tests/src/Phase1PreparedPlaybackTests.cpp`
  - source-path relink invalidates exactly one prepared asset
  - checksum-only change at the same path invalidates exactly one prepared asset
  - loop-policy-only change at the same path and checksum invalidates exactly one prepared asset
  - compiler/version salt changes produce a new prepared cache key
- guard regressions remained green in:
  - `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - `tests/src/Phase1DraftPlaybackFacadeTests.cpp`

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_draft_playback_facade_tests`
