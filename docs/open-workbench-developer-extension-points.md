# Open Workbench developer extension points

## Pass 03 component boundaries

- `PerformancePanel` owns Perform identity, readiness, disclosures, artwork priority, keyboard placement, and shell-level responsive composition.
- `PerformanceMixer` owns published-control lanes, value-only refresh, control focus order, and internal overflow. It receives immutable view data and reports value changes; it does not own authored Macro definitions.
- `MacroWorkbenchView` owns Macro list/definition/assignment geometry and paint. `AuthoringPanel` remains the coordinator for selection, callbacks, and `AuthoringSession` transactions.
- `RoutingWorkbenchView` owns signal-path/selected-insert geometry and paint. `AuthoringPanel` remains the coordinator for DSP scope, bus/FX selection, callbacks, and transactions.
- `OpenWorkbenchVisualSystem.h` is shared by Perform and authoring. These surfaces must not introduce a second look-and-feel or a local theme branch.

Component extraction must not move document state, undo ownership, host state, serialization, DSP, or audio-thread work into a view. Layout snapshots are read-only qualification seams, not a second source of UI state.

## Visual tokens

All Pass 03 colors, state treatments, radii, border widths, row/control heights, and type sizes belong in `app/src/shared/authoring/OpenWorkbenchVisualSystem.h`. New components should consume semantic roles such as `selection`, `focus`, `information`, or `modulation`; they should not copy ARGB literals from another component.

Selection and keyboard focus are separate states. Orange denotes authored selection, while blue denotes keyboard focus. Group tints and crossfade colors are data-bearing and cannot replace labels or state outlines. Custom surfaces use one-pixel structure and 2–3px radii; docked adjacent panes remain square and shadow-free.

When adding a state, define the semantic role centrally, check its contrast against every surface it uses, add it to `drs.open_workbench.phase4`, and exercise its pointer, keyboard, disabled, and selected forms.

## Frozen Pass 03 control and layout conventions

The following conventions are the implementation baseline after Phase 9. Changing one is a new UI iteration, not incidental cleanup.

| Concern | Frozen convention |
| --- | --- |
| Geometry | One-pixel structure, 2px control radius, 3px panel radius, square docked pane edges, no decorative shadows. |
| State colors | Orange selection, blue focus, blue information, teal modulation, semantic success/warning/error. Color always supplements text. |
| Typography | Shared title, section, field, body, compact, and metadata sizes from `OpenWorkbenchVisualSystem.h`; units/defaults appear beside values. |
| Dense editors | Stable ordered list first, scoped detail second, secondary actions last; compact modes stack the same workflow instead of hiding it. |
| Map protection | Focused workbench preference 340px, maximum 360px, protected map minimum 160px, then internal scrolling. |
| Perform | Identity/readiness first, Instrument Controls ahead of artwork, keyboard anchored, Details secondary and scroll-safe. |
| Macros | Ordered Macro list, Identity & Host, Range, Assigned Targets; destructive actions use `error`. |
| Routing | Bus & Signal Path before Selected Insert; parameter and Macro-control context remain in selected-insert detail. |
| Accessibility | Every interactive disclosure, tab, editor, selector, slider, list, and action has stable ID, title, description, help text, focus order, disabled explanation, and focus return when content collapses. |
| Navigation state | Active tab, workbench height, map viewport, and list selection are session-local UI state and never enter authored/host state or undo history. |
| Refresh | Value-only Perform updates do not rebuild control topology; authoring edits use existing transactions and do not add message/audio-thread work. |

Run `tools/qualify-open-workbench-phase9.ps1 -Configuration Debug` for the functional/UI matrix and `-Configuration Release` for the same matrix plus the optimized 1,700-zone responsiveness authority. A single isolated retry is allowed for a native headless teardown failure; two failures stop qualification.

## Map render policy

`app/src/shared/authoring/ZoneMapRenderPolicy.h` is the only place that decides which semantic details appear at Overview, Working, and Detail zoom bands. Geometry, hit testing, and selection remain authoritative in `ZoneMapCanvas`; the render policy may suppress or simplify paint but must never simplify hit targets.

When adding map decoration:

1. decide whether it is structural, state-bearing, or data-bearing;
2. assign its semantic zoom bands in `ZoneMapRenderPolicy`;
3. cull against normalized viewport geometry before text or path construction;
4. keep Overview free of persistent per-zone labels;
5. preserve source bounds for selection, audition, marquee, and editing;
6. update the 642/1,000-zone profile and Phase 2/5 qualification targets.

Map navigation is session-local UI state. Zoom, origin, minimap position, and workbench size must not enter authored project state, host recall state, dirty tracking, or undo history.

## Workbench and accessibility

The internal model and component IDs use `Workbench`, not the retired `Drawer` compatibility name. New workbench tabs need a stable `authoringWorkbench…` ID, accessible title, description, help text, explicit focus order, focus restoration on collapse, and a short-host layout that preserves `minimumMapVisibleHeight`.

Interactive map chrome must be named, keyboard reachable, and have a reliable return path to the map. New shell layouts must be added to `drs.open_workbench.phase5` and the manual demonstration script before rollout.

## Iteration stop boundary

Pass 03 stops at the coherent light shell, map/workbench, Perform, Macro, and Routing surfaces. Branding/logo/iconography, a dark theme, library/browser/content discovery, new Macro or Routing capabilities, a routing graph editor, and a general UI sweep remain explicitly outside this baseline.
