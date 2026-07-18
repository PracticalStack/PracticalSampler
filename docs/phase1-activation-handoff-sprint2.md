# Phase 1 Activation Handoff

This note captures the next Sprint 2 slice for section 6.1 of `engineering-plan.html`: activating a prepared revision at a defined render-block boundary and retiring the previous activation away from the audio thread.

## What changed

- added a processor-owned performance activation mailbox with:
  - one active immutable render snapshot
  - one pending immutable render snapshot
  - fixed-capacity slot ownership
  - a retired-activation queue drained on the message thread
- moved shell-owned activation servicing behind `Processor::serviceMessageThreadWork()` so one message-thread seam now:
  - services `EngineFacade` background work
  - stages new performance activations when engine state changes
  - drains retired activation slots after the callback hands them back
- changed the audio callback to:
  - observe pending performance activation only at `processBlock()` entry
  - swap the pending activation into the active slot at the block boundary
  - clear existing performance voices deterministically when a new activation takes over
  - enqueue the superseded activation for later message-thread retirement instead of reclaiming it on the callback
- switched performance note rendering to consume the active immutable activation snapshot rather than pulling live session state directly from `EngineFacade` during note start
- preserved the prior audible bootstrap path by seeding the activation mailbox from the last-known-good preview/default runtime while publish is still idle

## Why this matters

This is the first real block-boundary activation seam in the processor path.

- message-thread work now decides what the next published render state should be
- the callback only performs a non-blocking ownership swap at the start of a block
- the previous activation is not destroyed on the callback
- later Sprint work can replace the current reference-runtime payload with richer prepared-path state without changing the ownership pattern again

## Validation

The focused regression slice now proves that:

- default performance playback remains audible after processor bootstrap
- a newly published revision becomes pending on the message thread first
- the callback is the only place that promotes that revision to active
- superseded activations leave explicit retirement backlog until message-thread servicing drains them
- realtime callback safety counters remain clean through the new handoff path

Validated with:

- `drs.phase1.realtime_safety`
- `drs.phase0.smoke`

This remains a Sprint 2 bridge. The activation seam is real, but the renderer still plays the reference-runtime bridge rather than the final two-context prepared-path sampler core planned for the later section 6.1 slices.
