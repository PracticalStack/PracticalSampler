# Phase 1 Sprint 3 Invalidation Scope Separation

This note captures `S3.4-T2` from the Sprint 3 companion plan on July 18, 2026.

Goal of this step: separate invalidation caused by source-affecting edits from invalidation caused by zone-only authoring edits.

## What changed

- the prepared cache-key contract is now explicitly documented in code as excluding zone-only authoring fields
- `Phase1PreparedPlaybackTests.cpp` now proves one concrete zone-only edit path where:
  - immutable snapshot digest changes
  - prepared digest changes
  - prepared zone content changes
  - prepared sample handles, streams, ownership records, and cache keys all remain unchanged
  - cache hits stay warm and `decodedBytes` remains zero

## Why this matters

Prepared assets should only be invalidated when the source-backed material or decode policy changes.

Zone-only edits such as:

- gain
- pan
- key mapping
- other zone normalization fields

still affect prepared playback content, but they should do so by rebuilding zone metadata around the same cached source assets rather than forcing a cold source decode.

## Verification

- `tests/src/Phase1PreparedPlaybackTests.cpp`
  - zone-only authoring edit preserves cache-key identity and asset reuse
  - sample-content edit still invalidates the correct prepared assets
- guard regressions remained green in:
  - `tests/src/Phase1PreparedPlaybackWorkerTests.cpp`
  - `tests/src/Phase1DraftPlaybackFacadeTests.cpp`

Validated with:

- `drs_phase1_prepared_playback_tests`
- `drs_phase1_prepared_playback_worker_tests`
- `drs_phase1_draft_playback_facade_tests`
