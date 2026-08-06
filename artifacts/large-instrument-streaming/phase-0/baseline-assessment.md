# Phase 0 Baseline Assessment

Date: 2026-08-05/06 (America/New_York)  
Code changes before assessment: none.

## Git state

`master` contained only these untracked user-supplied files:

- `large-instrument-streaming-iteration-plan.html`
- `validation/large-instrument-plan-desktop.png`
- `validation/large-instrument-plan-mobile.png`
- `validation/large-instrument-plan-phase0.png`

They were preserved.

## Build environment

Direct `cmake --preset vs2022-debug` from a normal PowerShell failed while linking JUCE `juceaide`:

`LINK : fatal error LNK1181: cannot open input file 'kernel32.lib'`

The documented `tools/bootstrap-windows.ps1` correctly enters `VsDevCmd.bat`; both Debug and Release then configured and built.

## Debug baseline

Command:

`powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1 -RunTests`

Result:

- Build completed.
- CTest discovered 172 tests.
- 65 tests failed in the initial full run.
- `drs.host_state.restore_stress` did not terminate after 912.69 seconds of continuous CPU and had no CTest timeout. The exact validated test process was terminated so CTest could finish; this predates streaming edits.
- Deterministic isolated failures:
  - `drs.phase0.smoke`: default Performance activation not installed.
  - `drs.phase1.prepared_playback`: decode-policy rejection expectation failed.
- Resident baseline:
  - `drs.sprint4.offline_renderer`: passed.
  - Golden data: `tests/baselines/sprint4-offline-render-baselines.txt`.

Isolated command:

`ctest --test-dir build\vs2022-debug --output-on-failure -R "^(drs.phase0.smoke|drs.phase1.prepared_playback|drs.sprint4.offline_renderer)$"`

## Release baseline

Command:

`powershell -ExecutionPolicy Bypass -File .\tools\bootstrap-windows.ps1 -Configuration Release -RunTests`

Result:

- Configure and `drs_all_tests` build completed.
- 118/172 tests passed; 54 failed or were not run.
- Several CTest executables are absent from the `drs_all_tests` dependency list.
- Several lifecycle/report binaries terminate with Windows stack-buffer/segfault status.
- Smoke, prepared-playback, activation, preview, diagnostics, and release-gate defects reproduce.
- Release runtime baseline guard passed; this does not override the broad red suite.

## Resident behavior reference

The existing resident sampler offline-render comparison is green. Its golden fixture is the parity reference until the common data-source contract adds streamed execution. Streaming parity must not weaken this baseline or its playback semantics.

## Corpus availability

No Accurate Salamander/Salamander Grand Piano SFZ or corpus was found in the repository or in `E:/Dev/Cpp/VST/DecentRhapsody/RawSamples`. The available raw corpus includes a jRhodes SFZ. Salamander-dependent qualification will remain explicitly unverified; synthetic/sparse/legal fixtures must complete first.
