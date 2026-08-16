# Practical Sampler

Practical Sampler is a JUCE-based standalone application and plug-in shell built around a HISE-powered sampler runtime.

This repository is currently in Phase 0, where the goal is to establish a reproducible Windows-first foundation: repository layout, dependency policy, toolchain requirements, architectural boundaries, and a buildable project skeleton.

## Phase 0 decisions

- Repository root: `DecentRhapsodyStudio/`
- Source hosting: GitHub
- Primary development platform: Windows
- Supported IDE: Visual Studio 2022 Community
- Build system: CMake
- Dependency strategy: vendored third-party source under `third_party/`
- Package managers: not used in Phase 0
- Product license: GPL-compatible open-source license, exact choice deferred

## Repository layout

- `app/` - product-owned standalone and plug-in shell code
- `content/` - product-owned HISE authoring content and runtime-facing asset layout
- `engine_adapter/` - product-owned boundary layer between the app and HISE
- `docs/` - architecture notes, decisions, and setup documentation
- `tests/` - smoke tests and validation harnesses
- `tools/` - local developer scripts and helper utilities
- `third_party/` - vendored external dependencies kept separate from product code

## Toolchain baseline

- Visual Studio 2022 Community with the `Desktop development with C++` workload
- MSVC `v143` toolset
- CMake `3.28+`
- Windows SDK `10.0.22621.0+`
- Git

Local validation on this machine:

- CMake `4.3.0`
- Windows SDK `10.0.26100.0`

## Current status

The repository structure and vendored dependency policy are in place. JUCE and HISE source snapshots have been imported under `third_party/`, and the Windows-first CMake bootstrap is now building:

- a JUCE standalone shell
- a JUCE VST3 shell
- a product-owned HISE frontend compile probe
- a product-owned adapter seam for frontend profile metadata and HISE project content path discovery
- a Phase 0 smoke-test executable wired into CTest for shell and engine bootstrap validation

The Windows-first bootstrap lives at:

- `CMakeLists.txt`
- `CMakePresets.json`
- `tools/bootstrap-windows.ps1`
- `tools/install-vst3-windows.ps1`
- `docs/windows-build.md`
- `docs/architecture-overview.md`
- `docs/host-validation.md`
- `docs/large-instrument-streaming-support.md`

The current automated baseline runs on GitHub Actions through:

- `.github/workflows/windows-phase0.yml`

Sprint 1 bootstrap work has now started in the product-owned runtime seam:

- `engine_adapter/include/drs/engine/RuntimeModel.h`
- `engine_adapter/include/drs/engine/RuntimeLoader.h`
- `content/runtime/phase1/reference-corpus/`
- `docs/phase1-runtime-contract.md`
- `docs/phase1-reference-corpus.md`

Sprint 2 import-pipeline work has now started in the same seam:

- `engine_adapter/include/drs/engine/SampleImport.h`
- `engine_adapter/include/drs/engine/RuntimeCompiler.h`
- `engine_adapter/include/drs/engine/RuntimeLoadProfile.h`
- `engine_adapter/include/drs/engine/RuntimeStreamingService.h`
- `engine_adapter/include/drs/engine/RuntimeStream.h`
- `engine_adapter/include/drs/engine/RuntimeVoice.h`
- `engine_adapter/src/SampleImport.cpp`
- `engine_adapter/src/RuntimeCompiler.cpp`
- `engine_adapter/src/RuntimeLoadProfile.cpp`
- `engine_adapter/src/RuntimeStreamingService.cpp`
- `engine_adapter/src/RuntimeStream.cpp`
- `engine_adapter/src/RuntimeVoice.cpp`
- `docs/phase1-import-policy.md`
- `docs/phase1-load-profile.md`
- `docs/phase1-note-routing.md`
- `docs/phase1-pipeline-report.md`
- `docs/phase1-runtime-counters.md`
- `docs/phase1-streaming-service.md`
- `docs/phase1-stream-reader.md`
- `docs/phase1-voice-runtime.md`
- `tests/src/Phase1LoadProfileTests.cpp`
- `tests/src/Phase1NoteRoutingTests.cpp`
- `tests/src/Phase1RuntimeCountersTests.cpp`
- `tests/src/Phase1SampleImportTests.cpp`
- `tests/src/Phase1CompilePathTests.cpp`
- `tests/src/Phase1PipelineReport.cpp`
- `tests/src/Phase1StreamingServiceTests.cpp`
- `tests/src/Phase1StreamReaderTests.cpp`
- `tests/src/Phase1VoiceRuntimeTests.cpp`

Sprint 4 state-recall work has now started in the same seam:

- `engine_adapter/include/drs/engine/RuntimePresetState.h`
- `engine_adapter/src/RuntimePresetState.cpp`
- `content/runtime/phase1/preset-state/`
- `docs/phase1-diagnostics.md`
- `docs/phase1-failure-handling.md`
- `docs/phase1-macro-map.md`
- `docs/phase1-macro-bridge.md`
- `docs/phase2-group-mixer-workflow.md`
- `docs/phase1-performance-surface.md`
- `docs/phase1-reference-corpus.md`
- `docs/phase1-preset-state.md`
- `docs/phase1-regression-automation.md`
- `docs/phase1-state-recall.md`
- `tests/src/Phase1DiagnosticsTests.cpp`
- `tests/src/Phase1FailureHandlingTests.cpp`
- `tests/src/Phase1MacroBridgeTests.cpp`
- `tests/src/Phase1PresetStateTests.cpp`
- `tests/src/Phase1StateRecallTests.cpp`

Sprint 5 packaging work has now started for the reference instrument milestone artifact:

- `content/runtime/phase1/reference-corpus/tiny-open-instrument/package-manifest.json`
- `content/runtime/phase1/benchmark-scenes/reference-playback-scene.json`
- `tools/package-phase1-reference-instrument.ps1`
- `tools/run-phase1-benchmark-scene.ps1`
- `docs/phase1-benchmark-scene.md`
- `docs/phase1-reference-corpus.md`
