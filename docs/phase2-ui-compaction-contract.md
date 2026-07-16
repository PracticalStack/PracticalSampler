# Phase 2 UI Compaction Contract

Last updated: July 16, 2026

## Sprint 1 Purpose

This contract captures the current authoring-shell baseline and the explicit sizing and behavior rules that Sprint 2 and Sprint 3 must preserve while the UI is decomposed.

## Baseline Shell Sizes

- Compact plug-in baseline: `820 x 700`
- Expanded standalone baseline in the current codebase: `860 x 760`
- Expanded standalone target for Sprint 2 prototypes: `1120 x 800`
- Expanded standalone provisional minimum for Sprint 2 validation: `900 x 700`

Sprint 1 keeps the current baseline sizes intact. The larger expanded target is a forward contract, not a Sprint 1 runtime change.

## Region Contract

- Summary strip target height band: `72-88 px`
- Compact inspector target width band: `270-300 px`
- Expanded inspector target width band: `320-360 px`
- Drawer tab strip reservation: `36 px`
- Current waveform detail baseline height: `150 px`
- Current map baseline height: `160 px` compact, `190 px` expanded
- Minimum visible map height during compaction work: `160 px`

## Ownership Contract

- `AuthoringPanel` remains the workspace coordinator and the only class that mutates `AuthoringSession`.
- `AuthoringSummaryStrip` owns the summary-strip labels and primary action buttons in Sprint 1.
- `ZoneMappingEditor` owns the zone-field sliders, loop toggle, and restore-root-key action in Sprint 1.
- `ZoneMapCanvas` owns map painting only in Sprint 1.
- `WaveformDetailView` owns waveform painting only in Sprint 1.
- Selection summary, zone field values, and drawer state are represented as explicit UI view-model/state structs.
- Temporary migration rule: editor-mode routing still lives behind the existing selector until drawer migrations land.

## Selection And Drawer Rules

- Zone selection must not silently mutate macro, routing, or performance selections.
- Drawer state is UI-only state and must not be serialized into the project document.
- Compact shell default drawer state: closed.
- Expanded shell default drawer state: open to Waveform.
- Until Sprint 4, drawer state is modeled but not yet rendered as a visible drawer host.

## Keyboard And Focus Rules

- Focus order must remain top-to-bottom within the active editor mode.
- Hidden editor controls must not receive focus.
- Preview, undo, redo, and save actions must stay reachable in both shell modes.
- Stable component IDs are required for automated characterization tests.

## Sprint 1 Interaction Audit

- Map click selection: not implemented in the current `ZoneMapCanvas`; scheduled for Sprint 3.
- Direct map drag adjustment: not implemented in the current `ZoneMapCanvas`; scheduled for Sprint 3.
- Multi-select editing: not implemented in the current authoring session or UI; must remain out of scope unless explicitly added to Gate A work.
- Zone field mutation, macro mutation, routing mutation, performance-bank mutation, preview, undo, redo, and save checkpoint: implemented in the current workspace and must remain reachable throughout decomposition.

## Characterization Evidence

Sprint 1 characterization tests must cover:

- Compact and expanded shells
- Mapping, Macros, Routing, and Performance modes
- Direct instantiation of `AuthoringSummaryStrip` and `ZoneMappingEditor` with fixture view models and callback wiring only
- Stable-ID lookup for the workspace root, map, waveform, selectors, and primary actions
- Bounds and visibility assertions for the current baseline layout
- Diagnostic image capture for each shell/mode combination
