# Phase 1 Preset State Contract

This note captures Sprint 4 task `P1-401`: the first product-owned boundary between persisted preset recall, authored runtime content, and transient diagnostics.

## Contract summary

Phase 1 now defines a strict preset payload with:

- `schemaName = "drs.presetState"`
- `schemaVersion = 1`
- `presetId`
- target instrument identity: `targetInstrumentId`, `targetInstrumentSchemaName`, and `targetInstrumentSchemaVersion`
- user-facing runtime choices: `loadProfileId`, `selectedArticulationId`, and `macroValues`
- optional human-readable `notes`

The contract is intentionally strict. Unknown top-level fields fail parsing so project-content copies and transient telemetry cannot silently drift into checked-in presets or host state chunks.

## What belongs in preset state

Persisted state is limited to choices that a user expects to hear or feel again after reload:

- the selected articulation
- the selected load profile
- current macro values
- the identity of the target instrument the preset was authored against

These are the fields the host shell should save and restore during Phase 1.

## What remains project content

The following stay in the authored `.drinst`, `.drsproj`, and `.drstrm` content instead of being copied into preset state:

- manifest and stream-container paths
- source project paths
- articulation, group, and zone definitions
- sample paths and prefetch tables
- validation notes and other authored content metadata

This keeps the preset payload small and prevents the host shell from becoming a second, drift-prone copy of the runtime manifest.

## What remains transient

The following remain runtime-only and must never be serialized into preset state:

- page-miss counters
- active-voice counts
- cache residency and purge telemetry
- integration-state strings
- last-failure snapshots and similar debug text

Those values are useful for diagnostics, but saving them would make state recall noisy and misleading.

## Versioning notes

Version `1` is intentionally narrow. It establishes the boundary before host wiring lands in `P1-402`.

If Phase 1 later needs additional persisted controls, the team should:

- add them only when they represent true user-facing recall state
- keep project-content and diagnostics fields out of the payload
- advance `schemaVersion` when a change is not backward-compatible

## Validation

`drs_phase1_preset_state_tests` now proves that:

- two checked-in golden preset fixtures round-trip exactly
- the fixtures validate against the reference instrument manifest
- a negative fixture that leaks `compiledStreamAssetPath`, `streamingMetrics`, and `lastFailure` is rejected
- capturing preset state from a richer runtime-session snapshot strips transient diagnostics before serialization
