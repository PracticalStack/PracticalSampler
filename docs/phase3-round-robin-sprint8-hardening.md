# Phase 3 Round Robin Sprint 8 - Hardening And Release Gate

Frozen July 26, 2026.

Sprint `3.1.3.8` closes the Round Robin stream with a release-style gate instead of a new feature
surface. The goal of this slice is to prove that the now-shared Round Robin and velocity-crossfade
topology behaves deterministically when we repeat the same import, persist the resulting authored
content, and run it through the real Preview/Publish path.

## What this slice hardens

- Repeated import of the mixed RR-plus-crossfade mono Rhodes fixture stays deterministic at the
  analysis, projection, and native-manifest layers.
- The authored native project produced from that fixture round-trips through `.drsproj` persistence
  without losing explicit Round Robin descriptors or same-slot crossfade relationships.
- The built native instrument produced from that same authored project round-trips through `.drinst`
  persistence with the same explicit Round Robin descriptors.
- The imported project can move through `EngineFacade` Preview and Publish without introducing
  runtime findings, while keeping matching preview/publish digests and route topology.
- The legacy explicit-object precedence seam is still covered, but now with a topology-valid full
  RR pool so the test exercises precedence rather than incomplete-pool rejection.

## Mixed corpus used for the gate

- `DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz`

This fixture is the correct Sprint 8 gate because it mixes both supported RR sequencing and the
supported linear-crossfade subset that now shares pairing topology with RR slot identity.

## Tests added or tightened

- Added `drs.phase3.round_robin_hardening`
- Tightened `drs.phase3.round_robin_schema_persistence` so legacy precedence coverage uses a valid
  three-slot pool while still proving that the explicit `roundRobin` object wins over conflicting
  legacy scalar fields

## Exit signal

If these tests stay green, Phase 3.2 can treat Round Robin as a stable extension point for later
import, mic, and authoring work without reopening the Phase 3.1.3 schema or runtime seams.
