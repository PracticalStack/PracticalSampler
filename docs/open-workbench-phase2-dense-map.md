# Open Workbench Phase 2 dense map semantics

Status: implemented August 13, 2026

Phase 2 makes the Zone Map usable with production-scale instruments. It adds
semantic rendering, stable group color, cached geometry with early culling, and
a persistent minimap without changing instrument data, transactions, undo,
host recall, or audio behavior.

## Semantic zoom contract

| Display scale | Level | Visible behavior |
| --- | --- | --- |
| 25–34% | Overview | Square muted group zones, major grid, and orange aggregate selection. Labels, handles, and crossfade decoration are not painted. |
| 35–89% | Working | Selected and delayed-hover labels, selected/secondary orange states, useful crossfade bands, and four range handles. |
| 90–200% | Detail | Selected/hover name plus root, key, velocity, and round-robin metadata; range and crossfade grips are distinct. |

Unselected persistent labels are absent in every band. Hover labels use a
300 ms delay. The discrete thresholds are centralized in
`ZoneMapRenderPolicy`, so paint and edit affordances cannot drift between
callers. Existing source-geometry hit targets remain available at Fit All for
compatibility, even when overview rendering suppresses their handles.

Group tints use a stable hash of the authoring `groupId` and a small muted
palette. Color is supplementary: selection outline, minimap state, labels, and
inspector context remain the authoritative identity cues.

## Geometry and culling

- `AuthoringZoneSummary` now carries the project's real `groupId`.
- `ZoneMapCanvas` builds one cached normalized rectangle and group tint per
  zone when summaries change.
- Paint and hit-test layout reject cached rectangles outside the normalized
  viewport before converting them to pixels.
- Active range/crossfade previews temporarily derive geometry from preview
  values, preserving gesture accuracy.
- Overview painting does no zone-label text work and uses square geometry.

The diagnostic surface exposes cache size, last visible-zone count, and current
semantic level for deterministic qualification without adding runtime logging.

## Minimap contract

`ZoneMapOverview` is a persistent child of the map with component ID
`authoringZoneMapMinimap`. It shows the full normalized pitch/velocity domain,
aggregated group bounds, selected zone marks, and the current viewport frame.

- Clicking outside the frame recenters without changing zoom or selection.
- Dragging the frame continuously updates and clamps the main viewport.
- The interaction frame expands to at least 6 px per dimension.
- Zoom, pan, resize, Fit All, Fit Selected, selection refresh, and topology
  refresh all synchronize through the same `ZoneMapViewState`.

## Qualification and performance

Validated with VS2022 x64 on the reference Windows development machine:

- `drs.open_workbench.phase2`
- `drs.open_workbench.phase1`
- `drs.open_workbench.phase0`
- `drs.phase2.authoring_ui`
- `drs.velocity_crossfade.zone_map`
- `drs.phase2.mapping_workspace`
- `drs.phase2.repeated_structure_density`
- `drs.ui.responsiveness_baseline`

The dedicated target covers semantic thresholds, a 1,000-entry cache, group
aggregation, minimap selection/navigation, Detail-mode culling, and a rendered
visual artifact. At 1120 × 520 with 1,000 zones, 30 Fit All paints averaged
15.98 ms in Debug and 7.64 ms in Release. Release therefore meets the
provisional typical-paint target of under 8 ms. The full responsiveness baseline
completed in 94.36 seconds. A scripted p95 frame profile remains part of Phase 5
hardening; the current Debug qualification enforces a conservative 25 ms
average ceiling.
