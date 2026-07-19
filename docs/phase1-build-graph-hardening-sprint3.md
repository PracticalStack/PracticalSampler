# Phase 1 Sprint 3 Build Graph Hardening

This note captures Sprint 3 task `S3.7-T1` from section 6.1 of `engineering-plan.html`: repair the clean-build graph so prepared playback and sample-import consumers link successfully from a fresh checkout.

## What changed

- `SampleImport.cpp` now builds once through a dedicated `drs_sample_import` static library instead of being recompiled ad hoc inside multiple test and shell targets.
- `drs_engine_adapter` remains responsible for prepared-playback logic, while concrete consumers that need sample import now link the dedicated sample-import seam explicitly.
- `drs_plugin_shell` now depends on `drs_sample_import` instead of compiling `SampleImport.cpp` directly.
- the direct test consumers that previously carried `SampleImport.cpp` source workarounds now link `drs_sample_import`
- `tests/CMakeLists.txt` now defines an aggregate `drs_all_tests` target so clean test builds have one explicit entry point
- `tools/bootstrap-windows.ps1` now builds `drs_all_tests` when `-RunTests` is used, instead of building only the smoke target before running full `ctest`

## Why this matters

- prepared-playback consumers now resolve `importSampleFile(...)` through one product-owned build seam instead of repeated per-target source injection
- the sample-import linker failure reported by the Sprint 3 review no longer appears when building `drs_phase1_sample_import_tests`
- bootstrap and CI-style test runs now have a real build target for the full executable test surface, which closes the clean-checkout gap between build and `ctest`

## Verification

Validated on Sunday, July 19, 2026 with:

- `cmake --build --preset build-debug --target drs_all_tests`
- `ctest --preset test-debug -R "drs.phase1.sample_import|drs.phase1.prepared_playback$|drs.phase1.prepared_playback_worker|drs.phase0.smoke" --output-on-failure`
