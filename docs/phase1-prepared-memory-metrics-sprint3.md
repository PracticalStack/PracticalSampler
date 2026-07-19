# Phase 1 Sprint 3 Prepared Memory Metrics

This note captures Sprint 3 task `S3.7-T4` from section 6.1 of `engineering-plan.html`: reconcile prepared-memory metrics with actual retained resources so shell-facing ownership and pressure numbers describe real residency.

## Metric contract

- `preparedBytes` is now the shell-facing retained prepared residency number for a built preview or publish result.
- `preparedOwnershipBytes` is the ownership/accounting view of that same retained residency, which keeps worker and cache-pressure math aligned with the public prepared result.
- `preparedSampleDataBytes` explains why those bytes exist by reporting the decoded PCM retained by prepared sample handles.
- `preparedCacheWorkingSetBytes`, `preparedCacheByteBudget`, and `preparedCacheResidentBytes` now reason from retained residency surfaces instead of older payload-size assumptions.

## What changed

- prepared build metrics now carry explicit comments and helper naming that tie `preparedBytes` to retained residency rather than generic payload size
- cache-pressure working-set policy now derives its lane-level baseline from `previewPreparedBytes` and `publishedPreparedBytes`, which are already aligned to retained decoded residency
- shell detail now reports `Prepared playback residency` with explicit resident-byte labels instead of a generic prepared-bytes line
- shell detail also reports whether resident bytes and ownership bytes agree for preview and publish, making mismatches visible immediately if the contract regresses
- facade and diagnostics tests now assert that residency, ownership, and retained sample-data byte counters stay aligned for prepared playback results

## Why this matters

- diagnostics, shell detail, and cache-pressure numbers now speak the same byte language as the worker’s retained ownership model
- future Sprint 4 renderer work can treat prepared residency counters as real retained audio footprint instead of a proxy for compiled stream payload size
- if a later change causes ownership and residency to diverge intentionally, the current regression coverage will force that decision to be explicit

## Verification

Validated on Sunday, July 19, 2026 with:

- `cmake --build --preset build-debug --target drs_phase1_prepared_playback_tests drs_phase1_diagnostics_tests drs_phase1_draft_playback_facade_tests drs_phase0_smoke_tests`
- `ctest --preset test-debug -R "drs.phase1.prepared_playback$|drs.phase1.diagnostics|drs.phase1.draft_playback_facade|drs.phase0.smoke" --output-on-failure`
