# Phase 1 Voice Runtime

This note captures the third Sprint 3 slice: the first product-owned voice state object.

## Current scope

The voice runtime now owns explicit state for:

- allocated zone, group, and articulation identity
- note and velocity
- macro snapshot values captured at allocation time
- stream cursor position in frames and payload bytes
- current page lease and page index when the voice is reading streamed data
- lifecycle state: idle, active, waiting, releasing, finished, or failed

This slice is still metadata-driven. It does not render audio yet. Its job is to make playback state explicit and testable before the sampler starts mixing real voices.

## Lifecycle

The current voice object supports:

- allocation against a concrete runtime zone
- advancing through prefetch-head frames
- waiting when it reaches a streamed page that is not ready yet
- resuming once the background scheduler has resolved that page
- beginning release
- finishing and cleaning up page leases without stale cursor leakage
- being reused for a later allocation once cleanup is complete

## Validation

`drs_phase1_voice_runtime_tests` now proves:

- a voice can allocate against the reference instrument and capture a macro snapshot
- the cursor advances through head data first and then waits correctly at the stream boundary
- the voice acquires a page lease once the scheduler resolves the next page
- multiple voices can run concurrently without leaving stale leases behind
- finished voices can be reused safely for a later allocation
- invalid zone requests fail with an actionable error

The shared `drs_phase1_pipeline_report` artifact now also includes a `voiceRuntime` section so CI exposes whether a reference voice can allocate, wait, resume, and finish cleanly.
