# Mini Sprint 4.7 — Deterministic Offline Render Conformance Evidence

Completed July 19, 2026.

## Outcome

The shared sampler core now has a shell-free deterministic offline harness. It supplies immutable render models, a sample rate, globally timestamped events, output duration, and host-style block partitions directly to `SamplerPlaybackContext`. No device, editor, processor shell, worker timing, or UI state participates in the result.

## Harness and artifacts

- `tests/support/Sprint4OfflineRenderHarness.h/.cpp` renders a complete stereo timeline through the core and returns the full samples plus lifecycle state.
- Canonical artifacts include quantized FNV-1a checksum, peak, RMS, first/last non-silent frame, lifecycle counters, and final voice counts.
- Analytical samples use an absolute tolerance of `1e-6`; summary floating-point values use `1e-8`; reviewed checksums and integer lifecycle values match exactly.
- Passing runs write nothing. Partition or baseline mismatches create reviewable JSON/text artifacts under `sprint4-offline-render-failures` in the test working directory.
- `--emit-baselines` prints proposed manifest rows for review but never overwrites the checked baseline.
- The schema, review rules, and intentional-update procedure are recorded in `docs/phase1-sprint4-offline-render-baseline.md`.

## Reviewed scenarios

The checked manifest contains 20 scenarios:

1. silence;
2. sample-accurate timing;
3. mono duplication and root/unity pitch;
4. stereo channel preservation;
5. authored sample start offset;
6. octave pitch;
7. velocity scaling;
8. gain scaling;
9. pan balance;
10. sample completion and final-frame behavior;
11. mixed accumulation;
12. note-off release envelope and completion;
13. forward-loop boundary;
14. multiple loop wraps;
15. polyphony;
16. deterministic voice stealing at 25 requests into 24 slots;
17. repeated-note ownership and release;
18. all-notes-off;
19. emergency reset;
20. a 5,000-frame mixed lifecycle timeline used for partition invariance.

## Partition invariance

The canonical 5,000-frame timeline is rendered with block sizes 32, 64, 127, 256, 512, and 1024. Every sample matches the partition-32 reference within `1e-6`. Voice starts, releases, completions, steals, drops, resets, activation/retirement outcomes, and final voice states match exactly. `renderedBlockCount` is deliberately excluded because it measures the selected partition rather than renderer behavior.

## Validation

Focused offline target:

```text
drs.sprint4.offline_renderer: PASS
20 reviewed scenarios; partitions 32/64/127/256/512/1024
```

Focused Sprint 4, entry-gate, and realtime matrix:

```text
13/13 passed
```

Debug aggregate build and full matrix:

```text
cmake --build build/vs2022-debug --target drs_all_tests --config Debug -j 4
Result: PASS

ctest --test-dir build/vs2022-debug -C Debug --output-on-failure
Result: 48/48 passed
```

The passing run produced no failure-artifact directory.

## Exit decision

Mini Sprint 4.7 exit criteria are met. Every required renderer behavior has deterministic offline proof, and equivalent event timelines remain equivalent across all required host partitions. Mini Sprint 4.8 may proceed.
