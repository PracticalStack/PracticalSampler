# Sprint 4 Entry Gate EG3 Completion Evidence

Completed July 19, 2026. Processor diagnostics now use an audio-safe numeric publication followed by
message-owned immutable snapshot publication. The ownership map is in
`phase1-sprint4-entry-diagnostics-ownership-contract.md`.

## Implementation evidence

- Audio callback identity is thread-local, so overlapping message work is never attributed to audio.
- Realtime counters and per-slot activation identities are atomic primitives.
- An odd/even publication sequence prevents the message thread from accepting a torn audio frame.
- Retirement metrics use atomic indices and byte totals; diagnostics never inspect mutable SPSC slot arrays.
- Authoring document reads, shared failure details, snapshot allocation, and immutable pointer replacement
  are message-thread operations; a non-audio reader may refresh state labels in its private value copy.
- UI/test readers atomically load `shared_ptr<const ProcessorRealtimeSafetySnapshot>`, receive a value copy,
  and may overlay a newer coherent primitive frame without touching shared strings or aggregates.

## Focused concurrency matrix

`drs_sprint4_entry_diagnostics_concurrency_tests` runs three threads together:

- 1,200 deterministic 64-sample audio callbacks;
- 360 message-service iterations with macro edits, note events, and activation churn;
- continuous UI polling of immutable diagnostic snapshots.

The test rejects odd or regressing publication sequences, regressing callback counts, impossible voice
counts/capacities, inconsistent activation identity, retirement byte residue, unknown state strings,
deadlock, callback violations, or a missed callback. Five consecutive local Windows Debug runs passed;
each completed all 1,200 callbacks and at least 15,000 concurrent UI reads.

## Automated race coverage

- `.github/workflows/thread-sanitizer.yml` configures a Clang Debug build with
  `DRS_ENABLE_THREAD_SANITIZER=ON`, builds the focused target, and runs it under ThreadSanitizer on Ubuntu.
- `.github/workflows/windows-phase0.yml` builds and directly runs the same deterministic target after the
  existing Windows bootstrap/smoke suite.
- EG5-T1 registers the focused target as `drs.sprint4_entry.diagnostics_concurrency` and includes it
  in `drs_all_tests`; the Windows workflow retains its direct deterministic run and Linux CI retains
  the ThreadSanitizer configuration.

ThreadSanitizer execution is delegated to Linux CI; local verification covers the deterministic Windows
regression, `drs.phase1.realtime_safety`, `drs.phase1.diagnostics`, and the EG2 activation-payload regression.
