# Phase 1 Realtime Safety Harness

Sprint 1 now adds an executable realtime-safety seam around the shared plugin processor path.

## What this step covers

- preload reference playback samples before the first audio callback
- reserve active-voice storage before realtime playback begins
- reuse scratch MIDI buffers inside `processBlock`
- record callback budget and observed callback duration
- record tracked realtime violations when the callback has to:
  - load reference playback samples
  - load an authoring preview sample
  - grow active-voice storage

## Product seam

`plugin::Processor` now exposes `ProcessorRealtimeSafetySnapshot`, which reports:

- callback count
- prepared block size
- reference-sample warmup state
- active-voice capacity state
- tracked audio-thread violation counts
- last and max callback duration
- nominal callback budget

This keeps the first realtime harness product-owned and testable without depending on JUCE debug-only guards.

## Regression coverage

`drs.phase1.realtime_safety` now verifies that the performance path:

- prewarms reference playback samples outside the callback
- renders the first host note without tracked audio-thread violations
- survives a burst of queued performance-surface notes without growing active-voice storage

## Sprint 4 extension

The original Sprint 1 harness was intentionally limited to known first-note hazards. Sprint 4 Entry
Gate EG4 now adds test-scoped allocation/deallocation detection and explicit guards for locks, waits,
file access, path resolution, sample/stream decode, large-resource destruction, final ownership release,
and callback deadlines. See `phase1-sprint4-entry-realtime-guard-contract.md` for the current contract.
