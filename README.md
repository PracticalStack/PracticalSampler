# Decent Rhapsody Studio

Decent Rhapsody Studio is a JUCE-based standalone application and plug-in shell built around a HISE-powered sampler runtime.

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

The current automated baseline runs on GitHub Actions through:

- `.github/workflows/windows-phase0.yml`

Sprint 1 bootstrap work has now started in the product-owned runtime seam:

- `engine_adapter/include/drs/engine/RuntimeModel.h`
- `engine_adapter/include/drs/engine/RuntimeLoader.h`
- `content/runtime/phase1/reference-corpus/`
- `docs/phase1-runtime-contract.md`
- `docs/phase1-reference-corpus.md`
