# Phase 1 Sprint 3 Prepared Resource Ownership

This note captures Sprint 3 task `S3.7-T3` from section 6.1 of `engineering-plan.html`: define what a prepared playback handle actually owns before Sprint 4 starts depending on the seam.

## Ownership contract

- `PreparedPlaybackSampleHandle` now owns immutable decoded PCM through a shared `decodedSampleData` payload that retains normalized channel buffers for playback reuse.
- `PreparedPlaybackStreamHandle` continues to own the ready-to-consume stream topology metadata needed to bind the prepared sample back to the compiled container layout.
- The prepared playback seam therefore uses a bounded hybrid contract: decoded sample audio is retained once in the cache and shared into immutable prepared results, while stream/container metadata stays lightweight and metadata-oriented.

## What changed

- cold prepared-cache misses now decode the source file, move the normalized channel buffers into a shared immutable prepared-sample payload, and retain that payload in the cache-backed sample handle
- warm cache hits now reuse real decoded sample data instead of recreating only metadata
- prepared sample-data byte metrics now derive from the retained normalized channel buffers rather than inferred `channelCount * frameCount` math
- cache ownership bytes now follow the retained decoded PCM footprint instead of the compiled stream payload size, which makes active and retired ownership accounting describe real prepared residency
- prepared serialization and prepared content digests intentionally remain metadata-only so diagnostics stay deterministic without embedding large PCM blobs in snapshot text output

## Why this matters

- Sprint 4 can now consume prepared playback results that already own a real playable audio resource
- warm prepared builds no longer depend on an implicit future re-decode step to become useful to a renderer
- ownership and cache-pressure numbers are now grounded in the resource the preparation seam actually retains

## Verification

Validated on Sunday, July 19, 2026 with:

- `cmake --build --preset build-debug --target drs_phase1_prepared_playback_tests drs_phase1_prepared_playback_worker_tests drs_phase1_diagnostics_tests drs_phase1_draft_playback_facade_tests`
- `ctest --preset test-debug -R "drs.phase1.prepared_playback$|drs.phase1.prepared_playback_worker|drs.phase1.diagnostics|drs.phase1.draft_playback_facade" --output-on-failure`
