# Sprint 4.3 Voice Pool And Scheduler Completion Evidence

Completed July 19, 2026.

## Implementation

- Added fixed-capacity `SamplerEventBlock` with stable in-place ordering and explicit overflow count.
- Added fixed-capacity `SamplerVoicePool` with 24 inline `SamplerVoice` slots and explicit
  free/active/releasing/finished states.
- Added sample-accurate range rendering and event application, repeated-note ownership, ordinary
  release commands, emergency reset, deterministic route selection, slot reuse, and stealing.
- Added primitive started/released/completed/stolen/dropped/reset and final slot-state counters.

## Focused matrix

`drs_sprint4_scheduler_tests` verifies:

- stable sorting and equal-offset insertion order;
- exact note-on and emergency-reset sample boundaries;
- rejection of unsorted and out-of-block raw event views without partial mutation;
- repeated note-ons and release of all voices owned by the same source note;
- ordinary all-notes-off and immediate emergency reset;
- first 24 notes without stealing and the 25th stealing the oldest active voice;
- oldest-releasing-before-active steal priority;
- finished-slot reuse without a false steal;
- 128 accepted events and deterministic rejection/counting of event 129;
- deterministic drops for invalid velocity and no matching route;
- zero allocation/deallocation during full event admission and maximum bounded scheduling/render.

## Automated integration

- Product target: `drs_sampler_core`
- Focused executable: `drs_sprint4_scheduler_tests`
- CTest name: `drs.sprint4.scheduler`
- Aggregate dependency: `drs_all_tests`

The aggregate build succeeded. The final Sprint 4 entry/core matrix passed 8/8 in 15.25 seconds:

- all five `drs.sprint4_entry.*` tests;
- `drs.sprint4.render_model`;
- `drs.sprint4.voice_kernel`;
- `drs.sprint4.scheduler`.

See [the scheduler contract](phase1-sprint4-voice-pool-scheduler-contract.md) and
[the voice-kernel contract](phase1-sprint4-voice-kernel-contract.md).
