# Sprint 4 Entry Gate EG1-T1 Red-Test Evidence

This note records completion of Sprint 4 Entry Gate task `EG1-T1`: add red tests using genuinely new WAV and FLAC sources whose paths and source ids are absent from the Phase 1 reference stream.

## Test seam

- Added `tests/src/Phase1Sprint4EntryAuthoredInputRedTests.cpp`.
- Added the focused build target `drs_sprint4_entry_authored_input_red_tests`.
- The executable accepts one format and one lane per invocation:
  - `wav preview`
  - `wav publish`
  - `flac preview`
  - `flac publish`
- Each scenario generates a new audio file in the test scratch directory, appends it as authored content, proves the immutable snapshot is valid, queues preparation on the requested worker lane, and expects the external source to become a retained prepared sample.

## Current red result

All four scenarios fail at the intended boundary:

- WAV / Preview: `missing-prepared-stream-sample`
- WAV / Publish: `missing-prepared-stream-sample`
- FLAC / Preview: `missing-prepared-stream-sample`
- FLAC / Publish: `missing-prepared-stream-sample`

The executable validates that a failure is specifically the known Phase 1 reference-stream membership dependency before reporting the red result. An unrelated failure therefore cannot masquerade as EG1 evidence.

## Registration policy

The red target is intentionally not registered with CTest or `drs_all_tests` yet. This preserves the established green regression baseline while keeping the entry-gate failure executable and reproducible. EG1 implementation should turn all four scenarios green; EG5-T1 will add the green target to aggregate discovery.

## Resolution

EG1-T2 through EG1-T6 removed the reproduced dependency. The test is now the green
`tests/src/Phase1Sprint4EntryAuthoredInputTests.cpp` matrix built as
`drs_sprint4_entry_authored_input_tests`. The original four WAV/FLAC Preview/Publish cases pass,
and the matrix now also covers container-free preparation, cold/warm reuse, relink, same-path
replacement, missing source, unsupported format, cancellation, and frozen shell propagation.

EG5-T1 subsequently registered the green target as `drs.sprint4_entry.authored_input` and added
it to `drs_all_tests`.

## Verification

Built in the Visual Studio developer environment with:

```powershell
cmake --build build/vs2022-debug --target drs_sprint4_entry_authored_input_red_tests
```

Executed the four scenarios directly from the generated Debug artifact. Each returned exit code `1` with the expected `EG1-T1 RED` message and `missing-prepared-stream-sample` cause.

The existing registered worker regression remains green:

```powershell
ctest --test-dir build/vs2022-debug -R "drs.phase1.prepared_playback_worker$" --output-on-failure
```
