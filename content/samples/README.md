# Native Sample Content

This directory is the product-owned, HISE-independent source-sample root for Practical Sampler.

Phase 0 establishes the path and ownership contract. Sample assets will be migrated here from the
temporary `content/hise_project/Samples/` migration input in a later phase. Until that migration is
complete, do not add new HISE project files, sample maps, scripts, presets, XML backups, or engine
metadata to this directory.

## Contract

- Files are source audio owned or explicitly licensed for Practical Sampler development.
- Paths referenced by native `.drsproj` and `.drinst` fixtures are relative to this directory or to
  the product's declared project content root; they must not reference `hise_project`.
- The native runtime consumes audio files directly or through product-owned package formats. HISE
  sample maps and HISE authoring conventions are not part of this contract.
- Any new fixture should document its stable identifier, source path, format, and provenance in the
  owning runtime fixture or a future native sample manifest.
