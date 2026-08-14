# Open Workbench Phase 0 baseline

Status: implementation baseline

This note freezes the current Zone Map behavior before the Open Workbench UI
iteration changes visible interaction or rendering. It complements the phased
implementation plan at the workspace root.

## Deterministic qualification fixtures

- Demonstration baseline: 642 zone summaries.
- Stress qualification: 1,000 zone summaries.
- Twelve stable group IDs are carried as parallel fixture metadata because
  `AuthoringZoneSummary` does not currently expose group identity.
- Both fixtures contain narrow one-key zones, overlapping velocity layers,
  crossfades, long labels, and two identical full-range pathological zones.
- Fixture generation is pure and does not read sample files, project state, or
  the audio engine.

## Current input contract retained during Phase 0

| Input | Current behavior |
| --- | --- |
| `Ctrl + wheel` over the map | Zoom around the pointer |
| Plain wheel | Forward to the parent component |
| Left drag on empty map while zoomed | Pan |
| Left drag on empty map at Fit All | Marquee selection |
| Left drag on a range or crossfade handle | Edit the active range |
| Double click a zone | Audition |
| Escape during an edit gesture | Cancel the edit |

Phase 1 will revise and expose the complete navigation contract, including the
temporary hand, middle-button pan, Fit All, Fit Selected, toolbar controls, and
pinned axes. Phase 0 intentionally does not change visible bindings.

## View-state seam

`ZoneMapViewState` owns normalized zoom/origin math, coordinate round trips,
pointer-anchor zoom, pixel pan, clamping, reset, and fit-bounds calculations.
`ZoneMapCanvas` continues to own JUCE events, gesture precedence, drawing, and
hit testing.

View movement remains ephemeral UI state. It must not create an authoring undo
transaction, modify instrument content, enter host recall state, or call the
audio engine.

## Existing visual-token mapping

Phase 0 records the migration rather than changing appearance:

| Existing role | Open Workbench token target |
| --- | --- |
| Authoring panel background | `shell` (`#E9ECE8`) |
| Authoring card/control surface | `raised` / `surface` |
| Zone Map background | `map` (`#FBFAF5`) |
| Primary text/focus outline | `graphite` (`#273035`) |
| Control outline | `border` (`#BAC2C1`) |
| Authored selection/accent | `selection` (`#D76828`) |
| Informational state | `info` (`#2B76B7`) |
| Valid modulation | `teal` (`#238E83`) |
| Healthy validation | `success` (`#5E8D45`) |

The token source and visible palette conversion belong to Phase 4.

## Validation record

Validated in the VS2022 Debug configuration on August 13, 2026:

- `drs.open_workbench.phase0`
- `drs.phase2.authoring_ui`
- `drs.velocity_crossfade.zone_map`
- `drs.phase2.mapping_workspace`
- `drs.phase2.repeated_structure_density`
- `drs.ui.responsiveness_baseline`

All listed targets passed. The responsiveness baseline completed in 95.59
seconds; Phase 0 introduced no new release budget and made no audio-path change.
