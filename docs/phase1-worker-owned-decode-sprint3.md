# Phase 1 Sprint 3 Worker-Owned Decode

This note captures `S3.3-T2` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: move real WAV/FLAC source decode into the existing `PreparedPlaybackService` seam for Preview and Publish preparation, instead of letting prepared playback depend only on compiled stream metadata.

## What changed

- `PreparedPlaybackService::prepare(...)` now decodes cold-miss source samples through `importSampleFile(...)` before it materializes new prepared sample handles.
- cold cache misses now:
  - verify that the decoded source matches the compiled stream on checksum, format, sample rate, frame count, and channel count
  - populate prepared sample metadata from the decode path where that metadata is stable to preserve
  - count decoded source bytes through `PreparedPlaybackMetrics::decodedBytes`
- warm cache hits still reuse the existing immutable prepared handles and report `decodedBytes = 0`
- Preview and Publish both inherit this behavior automatically because they already converge through the same worker-owned preparation service

## Build ownership note

The product preparation seam now owns decode behavior, but the JUCE-backed importer implementation still compiles into the concrete engine-facing test executables rather than directly into `drs_engine_adapter`.

That keeps the Sprint 3 worker-boundary change focused on behavior first:

- `PreparedPlaybackService` owns when decode happens
- the focused engine-only regression binaries provide the existing importer object needed to exercise that seam

This is good enough for `S3.3-T2`. Broader library packaging of the decode implementation can stay separate from the worker-boundary contract itself.

## Verification

Focused coverage now proves the new seam in three places:

- `tests/src/Phase1PreparedPlaybackTests.cpp`
  - cold builds report decoded bytes
  - warm builds preserve cache determinism and report zero decoded bytes
  - migrated imported content also decodes through the preparation service
- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - cold Preview and cold Publish both decode through the worker lane
  - invalidation of one source re-decodes only the cold-miss handle
- `tests/src/Phase1DraftPlaybackFacadeTests.cpp`
  - facade-driven Preview and Publish flows still settle correctly through the background prepared worker after decode moved into the preparation seam

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_draft_playback_facade_tests`
