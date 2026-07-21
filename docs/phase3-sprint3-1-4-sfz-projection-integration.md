# Phase 3.1.4 - SFZ Projection and Authoring Integration

Date: Tuesday, July 21, 2026

Sprint 3.1.4 projects the reviewed SFZ analysis output into native authoring entities instead of stopping at the compatibility report.

Implemented in this sprint:

- Added `SfzImportProjection` to turn analyzed SFZ regions into native `RuntimeProjectSampleSource` and `RuntimeProjectZoneDefinition` content.
- Reused the existing authoring document controller through `applySfzImportProjection(...)` so reviewed imports land as a single undo-safe commit.
- Persisted SFZ provenance and compatibility notes into `project.notes` and `authoring.notes` when the reviewed import is applied.
- Extended native project, snapshot, prepared-playback, and instrument-manifest models with:
  - `releaseSeconds`
  - `roundRobinLength`
  - `roundRobinPosition`
- Preserved round-robin metadata through `.drsproj` and `.drinst` serialization.
- Updated native runtime routing so repeated note starts can resolve distinct round-robin positions instead of collapsing to a single imported region.
- Updated sampler voice release handling so imported `ampeg_release` values can drive native release timing while preserving the existing compatibility fallback when no imported release is present.

Coverage added:

- `Sprint31SfzProjectionTests.cpp`
  - verifies projection of the first SFZ fixture
  - verifies persisted project and authoring notes
  - verifies undo/redo for the applied import
  - verifies `.drsproj` and `.drinst` round-tripping
  - verifies native round-robin routing remains resolvable after conversion

Focused validation run on Tuesday, July 21, 2026:

- `drs.sprint31.sfz_contract`
- `drs.sprint31.sfz_fixture_profile`
- `drs.sprint31.sfz_parser`
- `drs.sprint31.sfz_normalization`
- `drs.sprint31.sfz_compatibility`
- `drs.sprint31.sfz_report_model`
- `drs.sprint31.sfz_projection`
