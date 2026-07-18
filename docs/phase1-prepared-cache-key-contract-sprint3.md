# Phase 1 Sprint 3 Prepared Cache Key Contract

This note captures `S3.4-T1` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: define prepared cache keys from stable identity and policy inputs instead of ad hoc field selection.

## What changed

- `PreparedPlaybackService` now builds prepared cache keys from four explicit components:
  - canonical source identity
  - source fingerprint
  - decode policy fingerprint
  - compiler/version salt
- canonical source identity remains:
  - `sampleSourceId + "|" + normalized source path`
- source fingerprint remains the compiled stream checksum that the worker also validates against decode output
- decode policy fingerprint now explicitly includes:
  - format name
  - sample rate
  - channel count
  - channel layout
  - payload encoding
  - page size
- immutable prepared notes now record the cache-key contract in plain text so the contract is visible in serialized prepared output

## Why this matters

This separates three different invalidation reasons cleanly:

- identity changes
  - same file content assigned to a different sample-source identity should not silently reuse the wrong prepared asset
- content changes
  - a fingerprint change should invalidate the prepared asset even if the path stays the same
- policy changes
  - streaming/decode policy shifts such as page-size changes should invalidate the prepared asset even when source content is unchanged

The compiler/version salt remains the final escape hatch for intentional contract evolution.

## Verification

- `tests/src/Phase1PreparedPlaybackTests.cpp`
  - repeated builds preserve the same cache keys for unchanged content
  - two sample sources that point at the same file preserve the same fingerprint but still receive different cache keys because canonical source identity differs
  - a page-size policy shift changes the prepared cache key while preserving source identity and fingerprint
  - a compiler/version salt shift changes the prepared cache key without changing source identity or decode policy metadata
- guard regressions remained green in:
  - `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - `tests/src/Phase1DraftPlaybackFacadeTests.cpp`

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_draft_playback_facade_tests`
