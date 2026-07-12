# Phase 1 Compile Path

This note captures the second Sprint 2 slice: the first deterministic compiler pass that turns imported sample metadata into runtime-facing artifacts.

## Current scope

The compiler seam now has two responsibilities:

- build product-owned `RuntimeProjectModel` and `RuntimeInstrumentModel` instances from an explicit compile plan
- emit a prototype `.drstrm` descriptor with deterministic payload offsets, prefetch sizes, and page-table placeholders

The current implementation is intentionally modest. It does not yet write a binary stream container. Instead, it freezes the vocabulary and structure that later binary container work will need.

## What the compiler consumes

The compile plan is explicit about:

- output project, instrument, and stream paths
- project and instrument identity
- imported sample metadata for each source asset
- articulation, group, macro, and zone layout
- page size and per-zone prefetch size

The compile seam now also applies the same Phase 1 sample-policy vocabulary used by the importer, including hard rejection of unsupported sample metadata and content-layout violations plus warning propagation for non-portable sample names.

That means the compile path can stay product-owned even while the decode seam is still JUCE-backed.

## Validation

`drs_phase1_compile_path_tests` now proves three things:

- compiling the same reference input twice yields byte-identical `.drsproj`, `.drinst`, and prototype `.drstrm` text artifacts
- the compiler-generated project, instrument, and stream descriptor artifacts still match the checked-in reference golden files
- the Sprint 1 loader can open compiler-generated manifests written to a temp directory from real source audio inputs
- unsupported sample metadata and content-layout violations fail loudly instead of producing silent bad artifacts

## Deliberate limitation in this slice

The compile-path tests currently compile `SampleImport.cpp` inside the JUCE-backed test target rather than linking the importer implementation through `drs_engine_adapter` directly. This keeps the existing adapter library stable while the project decides how JUCE module-backed decode support should live in product code longer term.
