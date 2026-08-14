# Open Workbench Phase 1 navigation

Status: implemented August 13, 2026

Phase 1 completes the first user-visible Open Workbench slice. It builds on the
normalized `ZoneMapViewState` introduced in Phase 0 and does not change authored
instrument data, undo history, host recall state, or audio behavior.

## Navigation contract

| Input | Behavior |
| --- | --- |
| Stepped mouse wheel | Zoom around the pointer |
| `Ctrl/Cmd + wheel` or compatible pinch event | Zoom around the pointer |
| Smooth two-axis wheel/trackpad event | Pan on both reported axes |
| `Shift + wheel` | Pan horizontally |
| Middle-button drag | Temporary hand pan |
| `Space + left drag` | Temporary hand pan while Space is held |
| Plain left drag on empty map | Marquee selection, including while zoomed |
| Drag on a range or crossfade handle | Existing authored edit gesture; handle precedence is retained |

Range and crossfade gestures remain authoritative when a gesture starts on an
active handle. Navigation is disabled while an authored edit or marquee is in
progress.

## Map toolbar

The toolbar is owned and laid out by `ZoneMapCanvas`, so it does not consume a
new row in `AuthoringPanel` or change the panel's minimum map-height contract.

- **Fit All** restores the complete MIDI 0–127 and velocity 1–127 extent.
- **Fit Selected** frames the union of selected zone bounds with padding.
- **− / +** zoom around the viewport center.
- The zoom readout presents the existing 1x–8x transform as the approved
  25%–200% user-facing scale.
- Toolbar commands return keyboard focus to the map after activation.

Component IDs:

- `authoringZoneMapToolbar`
- `authoringZoneMapFitAll`
- `authoringZoneMapFitSelected`
- `authoringZoneMapZoomOut`
- `authoringZoneMapZoomIn`
- `authoringZoneMapZoomValue`

## View lifecycle

- Refreshing the same ordered zone-ID topology preserves zoom and origin.
- A material topology change resets to Fit All.
- Selection changes preserve the viewport.
- Fit and navigation operations preserve primary and additional selection.
- All viewport operations remain UI-only and do not enter the authoring
  transaction path.

## Pinned axes

The map reserves a compact internal toolbar band, a left velocity axis, and a
bottom pitch axis. Axis positions stay fixed while their tick labels reflect the
visible content range. Zone geometry now uses interval boundaries (`key / 128`
through `(key + 1) / 128`) so the outer MIDI 0 and MIDI 127 zones remain visible
and selectable at Fit All.

## Validation record

Validated in the VS2022 Debug configuration on August 13, 2026:

- `drs.open_workbench.phase1`
- `drs.phase2.authoring_ui`
- `drs.velocity_crossfade.zone_map`
- `drs.phase2.mapping_workspace`
- `drs.phase2.repeated_structure_density`
- `drs.ui.responsiveness_baseline`

The dedicated Phase 1 target covers toolbar state, Fit Selected, Fit All outer
edges, topology preservation/reset, smooth pan, horizontal pan, display-scale
clamping, axis reservation, and a 642-zone visual-check render. The final
responsiveness baseline completed in 96.11 seconds.
