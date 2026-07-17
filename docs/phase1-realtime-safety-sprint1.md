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

## Known limits

Sprint 1 does not yet provide a universal detector for every possible heap allocation, lock wait, or host-side stall inside the callback. This harness is intentionally narrower: it hardens the known first-note hazards in the shared processor path and makes future regressions visible in CI.
