# Phase 1 Sprint 3 Deterministic Prepared Content

This note captures `S3.3-T3` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: keep prepared playback content and comparable metrics deterministic between cold and warm builds while still preserving true per-build decode work.

## What changed

- `PreparedPlaybackMetrics` now exposes `preparedSampleDataBytes` as the stable float-decoded footprint represented by the prepared sample handles.
- cold and warm builds now agree on:
  - prepared digest
  - prepared content serialization
  - prepared sample/stream/zone/ownership counts
  - prepared sample-data footprint
- `decodedBytes` remains intentionally operational:
  - cold cache misses report actual worker-owned decode work
  - warm cache hits keep `decodedBytes = 0`

## Why this split matters

Sprint 3 now distinguishes two different truths:

- deterministic prepared content size, which should be comparable across equivalent builds
- transient preparation work, which should differ between cold and warm execution

That keeps contract checks stable without hiding whether a build really had to decode source material.

## Verification

- `tests/src/Phase1PreparedPlaybackTests.cpp`
  - cold and warm prepared builds preserve matching `preparedSampleDataBytes`
  - invalidation keeps deterministic sample-data accounting aligned with the rebuilt prepared content
  - migrated imported content preserves the same cold-versus-warm metric split
- `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - repeated preview preparation through the worker keeps digest and `preparedSampleDataBytes` stable while `decodedBytes` drops to zero on the warm pass
  - cold Publish preparation reports the same prepared sample-data footprint as cold Preview for equivalent source content

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_draft_playback_facade_tests`
