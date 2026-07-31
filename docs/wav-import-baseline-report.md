# WAV Import Baseline Report

Sprint 1 recorded the pre-async synchronous WAV-import behavior in two places:

- `drs_wav_import_baseline_report` generates a fresh JSON artifact in the test build directory.
- `validation/wav-import/sync-shell-baseline.json` is the checked-in July 31, 2026 reference snapshot.

## Why this exists

The generated artifact preserves the measured constructor, project-replace, restore, import-submit,
full-batch, and memory-shape diagnostics from the legacy synchronous path.

The checked-in snapshot gives later work a stable historical reference for the old synchronous path:

- constructor, replace, and restore still performed import-related sample I/O;
- chooser completion still performed copy, fingerprint, reader-open, and full-frame decode work inline; and
- the retained queue and estimated peak working bytes were explicitly captured before the async
  service changed the shape.

The shipped product no longer uses that synchronous shell workflow. The final release evidence and
current async-only gates are recorded in [wav-import-release-evidence.md](wav-import-release-evidence.md).

## Current baseline shape

- `constructor`, `projectReplace`, and `restore` report timing plus import I/O counters
- `importSubmit` reports the historical synchronous chooser callback path
- `fullBatch` is identical to `importSubmit` in that historical baseline because the old shell
  drained the batch before the callback returned
- `memoryShape` records the largest decoded sample plus retained and peak working-byte estimates

## Refresh workflow

1. Build `drs_wav_import_baseline_report`.
2. Run `ctest -C Debug -R "drs.wav_import.baseline_report" --output-on-failure`.
3. Review `build/vs2022-debug/tests/wav-import-baseline-report.json`.
4. Refresh `validation/wav-import/sync-shell-baseline.json` only after confirming the drift is
   intentional.
