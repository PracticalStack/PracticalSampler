# Phase 0 Decisions

This document records Phase 0 decisions that are settled enough to guide repository setup.

## Confirmed

- Repository root is `DecentRhapsodyStudio/`.
- The `_analysis/Rhapsody` tree is reference material, not product-owned source.
- GitHub is the planned source hosting platform.
- Windows is the first fully supported development platform.
- Visual Studio 2022 Community is the supported IDE for early development.
- CMake is the build system.
- Package managers are intentionally avoided in Phase 0.
- Practical Sampler is moving to a product-owned native runtime with no HISE dependency.
- The current HISE vendor tree and `content/hise_project/` are temporary migration inputs only; no
  new product code may depend on them.
- Native source samples belong under `content/samples/`, and native runtime fixtures belong under
  `content/runtime/`.
- `_analysis/Rhapsody/` remains reference material and is explicitly outside the HISE-removal scope.
- The eventual project license will be GPL-compatible and open source, but the exact license is deferred for now.

## Working toolchain baseline

- Visual Studio 2022 Community
- `Desktop development with C++` workload
- MSVC `v143`
- CMake `3.28+`
- Windows SDK `10.0.22621.0+`
- Git

## Still deferred

- Exact GPL-compatible license selection
- Specific plug-in format validation target and host
- Exact pinned upstream commits for JUCE and HISE
- Detailed contributor/governance collateral beyond what is needed for private early development
