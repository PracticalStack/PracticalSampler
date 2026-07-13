# Phase 1 State Recall

This note captures Sprint 4 task `P1-402`: wiring the Phase 1 preset-state contract into the real standalone and plugin shell save/restore paths.

## Current scope

Both product-owned shells now round-trip the same JSON preset payload defined in `docs/phase1-preset-state.md`.

The current implementation uses:

- `EngineFacade` as the single owner of the active runtime session state
- `PluginProcessor::getStateInformation()` and `setStateInformation()` for the host/plugin path
- `MainComponent::exportStateJson()` and `restoreStateJson()` for the standalone path

The saved payload currently persists:

- target instrument identity and schema version
- selected load profile
- selected articulation
- macro values
- preset id

## Failure behavior

Restore is intentionally validating, not permissive.

If incoming state:

- fails JSON parsing
- targets the wrong instrument or schema version
- references an unknown load profile
- references an unknown articulation
- leaks project-content or transient diagnostic fields

then the shell keeps the previous valid session state and records a transient restore failure for diagnostics.

## Validation

`drs_phase1_state_recall_tests` now proves:

- the standalone shell can restore the checked-in lead/performance fixture, export it again, and reload it into a fresh shell instance
- the plugin processor can restore the same fixture, serialize it through JUCE host state hooks, and reload it into a fresh processor instance
- invalid restore payloads do not overwrite the last known-good shell state

This gives Phase 1 a real save/reload seam before broader diagnostics and host QA land in later Sprint 4 slices.
