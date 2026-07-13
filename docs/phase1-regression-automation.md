# Phase 1 Regression Automation

This note captures Sprint 4 task `P1-406`: expanding the shared Phase 1 regression artifact so CI covers more than initial load and playback.

## Current scope

`drs_phase1_pipeline_report` now extends beyond loader and playback checks. It also validates:

- standalone save and reload against the checked-in lead/performance preset fixture
- plugin host-state save and reload through `getStateInformation()` / `setStateInformation()`
- macro-state compare checks between restored runtime macro values and host-facing `macro.*` parameters
- missing-pack/content, checksum, schema, and partial-artifact handling through the product-owned shell failure probes

## Nightly summary

The report now carries a `nightlyValidation` block with separate pass/fail signals for:

- `load`
- `play`
- `stateRecall`
- `errorHandling`

This keeps the CI artifact easy to scan when a regression lands. The team can tell whether the break came from content load, playback/runtime behavior, state recall, or failure handling without reading every lower-level section first.

## Validation

`drs_phase1_pipeline_report` now proves that:

- save/reload still round-trips through both shells using the real Sprint 4 state boundary
- invalid restore payloads still preserve the last known-good state
- restored plugin macro values still match the host-facing parameter surface
- missing-pack/content and the other planned negative cases still fail gracefully and remain visible in diagnostics
