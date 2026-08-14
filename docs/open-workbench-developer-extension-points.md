# Open Workbench developer extension points

## Visual tokens

All authoring colors, state treatments, radii, border widths, row/control heights, and type sizes belong in `app/src/shared/authoring/OpenWorkbenchVisualSystem.h`. New components should consume semantic roles such as `selection`, `focus`, `information`, or `modulation`; they should not copy ARGB literals from another component.

Selection and keyboard focus are separate states. Orange denotes authored selection, while blue denotes keyboard focus. Group tints and crossfade colors are data-bearing and cannot replace labels or state outlines. Custom surfaces use one-pixel structure and 2–3px radii; docked adjacent panes remain square and shadow-free.

When adding a state, define the semantic role centrally, check its contrast against every surface it uses, add it to `drs.open_workbench.phase4`, and exercise its pointer, keyboard, disabled, and selected forms.

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
