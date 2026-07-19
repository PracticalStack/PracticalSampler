# Sprint 4.4 Voice Lifecycle Completion Evidence

Completed July 19, 2026.

## Implementation

- Extended `SamplerVoice` with explicit releasing lifecycle, fixed compatibility-release counters,
  loop activation, loop-edge interpolation, modulo fractional wrapping, and release completion.
- Connected pool note-off and all-notes-off commands to idempotent `beginRelease()`.
- Preserved fixed slot reuse/steal/reset behavior without shared ownership in voice state.
- Updated the existing scheduler reference vector to include the first two compatibility-release
  coefficients after its sample-accurate note-off.

## Focused matrix

`drs_sprint4_voice_lifecycle_tests` verifies:

- unity forward-loop sequences and repeated wraps;
- interpolation from the last loop frame back to loop start;
- fractional cursor wrap when a voice starts inside the loop;
- increments crossing a loop multiple times and valid one-frame loops;
- natural post-loop tail when start offset is at/after loop end;
- exact 2,048-sample release duration, first/second/final coefficients, and completion;
- repeated note-off does not restart release;
- combined loop/release output and lifecycle invariance across 1/31/256/512/1248 partitions;
- pool note-off timing and release completion into finished state;
- natural non-looping completion;
- oldest-releasing stealing, repeated emergency reset, post-reset silence, and unchanged model
  ownership count.

## Automated integration

- Product target: `drs_sampler_core`
- Focused executable: `drs_sprint4_voice_lifecycle_tests`
- CTest name: `drs.sprint4.voice_lifecycle`
- Aggregate dependency: `drs_all_tests`

The aggregate build succeeded. The final Sprint 4 entry/core matrix passed 9/9 in 15.06 seconds:

- all five `drs.sprint4_entry.*` tests;
- `drs.sprint4.render_model`;
- `drs.sprint4.voice_kernel`;
- `drs.sprint4.scheduler`;
- `drs.sprint4.voice_lifecycle`.

See [the lifecycle contract](phase1-sprint4-voice-lifecycle-contract.md),
[the scheduler contract](phase1-sprint4-voice-pool-scheduler-contract.md), and
[the voice-kernel contract](phase1-sprint4-voice-kernel-contract.md).
