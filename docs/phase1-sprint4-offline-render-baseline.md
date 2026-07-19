# Sprint 4 Offline Render Baseline Contract

This contract governs `drs.sprint4.offline_renderer` and the reviewed baseline in `tests/baselines/sprint4-offline-render-baselines.txt`.

## Artifact format

The baseline manifest is a versioned, line-oriented text format. Each scenario records:

- total rendered frames;
- a 64-bit FNV-1a checksum over stereo samples quantized at `1e-7`;
- peak and RMS summaries;
- first and last non-silent frames using a `1e-6` silence threshold;
- render, voice-start, release, completion, steal, dropped-event, reset, activation, retirement, and final voice-state counters.

Failure artifacts use `drs.sprint4.offline-render-artifact` JSON version 1. They include the scenario, partition, tolerances, summary, lifecycle counters, and complete stereo output. A companion mismatch report identifies the first differing channel/frame and expected versus actual value.

## Comparison rules

- Analytical golden assertions use an absolute sample tolerance of `1e-6`.
- Baseline peak/RMS summaries use an absolute tolerance of `1e-8`.
- The quantized checksum, frame count, non-silent bounds, and lifecycle outcomes must match exactly.
- Partition invariance compares every output sample and all behavior-dependent lifecycle counters. `renderedBlockCount` is intentionally excluded because it is defined by partition count rather than renderer behavior.
- The canonical invariance render uses partition 32. Partitions 64, 127, 256, 512, and 1024 must match it.

## Failure artifacts

A passing test writes no artifacts. On a partition or baseline mismatch, the executable creates `sprint4-offline-render-failures` beneath its working directory and writes only the files needed to review that mismatch.

## Intentional baseline update workflow

1. Run the failing test and inspect the analytical assertion or generated expected/actual artifacts first.
2. Determine whether the change is a defect, unsupported floating-point drift, or an intentional renderer contract change.
3. For intentional changes, run `drs_sprint4_offline_renderer_tests --emit-baselines` and review its proposed manifest rows without overwriting the checked baseline.
4. Update only the affected checked rows, keeping the schema version unchanged unless the artifact fields or interpretation changed.
5. Review the code change and baseline diff together. A checksum-only approval without output/lifecycle review is invalid.
6. Re-run the focused offline test, the Sprint 4 matrix, and the aggregate CTest matrix. Confirm the failure-artifact directory was not created by the passing run.

Baseline changes are never automatic during normal CTest execution.
