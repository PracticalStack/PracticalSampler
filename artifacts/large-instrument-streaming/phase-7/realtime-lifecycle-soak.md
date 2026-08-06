# Realtime and lifecycle soak

Result: PASS

Environment: Windows 11 Home 10.0.26200; AMD Ryzen 9 8945HS (16 logical processors); 33,593,020,416 bytes RAM; MSVC 19.44; CMake 4.3.0.

## Realtime guard

- Debug: `drs.sprint4_entry.realtime_guard` passed in 156.91 s.
- Release: `drs.sprint4_entry.realtime_guard` passed in 155.82 s.
- Release focused matrix: 8/8 passed in 156.59 s.
- Final Debug non-guard focused matrix: 26/26 passed in 17.30 s.
- The real-corpus qualification measured a maximum callback of 273 us against a 5,333 us budget, with zero normal-profile page misses or underrun frames.
- Final real-package host validation reported zero standalone and plug-in realtime violations.

## Performance semantics

The final matrix passes keyswitch selection/consumption, sustain pedal ordering and fixed-root pedal samples, release routing, choke, sequential round robin and reset, voice lifecycle/stealing, scheduler behavior, playback-context cutover, offline rendering, and velocity-crossfade mixing. The corrected full-velocity fixtures assert their actual 0 dB outputs while retaining their articulation, fixed-root, and one-slot-only routing predicates.

## Lifecycle and fault coverage

- `drs.phase1.prepared_playback_worker`: bounded queueing, mutation, cancellation, cache pressure, RF64, and worker shutdown.
- `drs.phase1.performance_package_session`: standalone/plug-in parity and package-session lifecycle.
- `drs.performance_package.export_lifecycle`: busy rejection, deterministic in-flight cancellation, terminal publication, idempotent shutdown, completion, and cleanup.
- `drs.host_state.restore_coordinator`: background restore preparation and nonblocking state entry.
- `drs.package_v2.records`: replacement, 100 cancel/replacement operations, degraded last-known-good behavior, corruption, cancellation, and staged-output cleanup.
- Real package host validation: editor creation/churn, plug-in and standalone activation, host save/restore with editor closed, post-restore editor creation, audible playback, and natural teardown.

The export lifecycle regression found during this soak was repaired by restoring the mandatory `writingStream` transition before `sealingPackage`; the lifecycle test then passed in both Debug and Release.

The JUCE GUI host-validation CTest is marked `RUN_SERIAL`: it passed isolated, but parallel process-level execution with another package session could collide in JUCE host initialization. With the test property applied, the same `-j 4` focused matrix passed 26/26.
