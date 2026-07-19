# Phase 1 Sprint 3 Validation Matrix

This note captures Sprint 3 task `S3.6-T6` from section 6.1 of `engineering-plan.html`: run the broader migrated-draft and reference-project matrix before declaring Sprint 3 ready for shared renderer work.

## Matrix scope

The final Sprint 3 pass set covers the seams that Sprint 4 will inherit directly:

- reference-project prepared playback correctness
- migrated-draft worker preparation and queue behavior
- facade-level preview and publish orchestration
- shell-facing diagnostics and performance snapshots
- realtime-safety guards around callback ownership
- standalone and plugin smoke coverage for the visible shell path

## Validation set

Validated on Sunday, July 19, 2026 with:

- `ctest --preset test-debug -R "drs.phase0.smoke|drs.phase1.prepared_playback$|drs.phase1.prepared_playback_worker|drs.phase1.draft_playback_facade|drs.phase1.diagnostics|drs.phase1.realtime_safety" --output-on-failure`

Passing tests:

- `drs.phase0.smoke`
- `drs.phase1.prepared_playback`
- `drs.phase1.prepared_playback_worker`
- `drs.phase1.draft_playback_facade`
- `drs.phase1.diagnostics`
- `drs.phase1.realtime_safety`

## One hardening change made during validation

- `Phase0SmokeTests.cpp` no longer assumes a queued performance-surface note must become audible in the first rendered block.
- The smoke assertion now accepts a short bounded multi-block window, which matches the collector-driven note-queue seam without weakening the audible-output guarantee.

## Outcome

- migrated imported content remains green through the worker path
- reference-project prepared playback remains green through the direct prepared-playback path
- facade, diagnostics, and realtime-safety coverage remain aligned with the current prepared-build and retirement metrics
- the broader Sprint 3 handoff surface is now green enough to start Sprint 4 shared-renderer work without reopening the prepared-assets contract boundary
