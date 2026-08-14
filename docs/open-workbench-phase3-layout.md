# Open Workbench Phase 3 workbench layout

Status: implemented August 13, 2026

Phase 3 converts the fixed authoring workbench into a session-local, resizable
workbench while preserving the existing content components, callbacks, command
handlers, and authored-state boundaries.

## Workbench states

`WorkbenchLayoutState` owns the UI-only height policy:

| State | Preferred height | Behavior |
| --- | ---: | --- |
| Collapsed | 38 px | Tab rail only. The previous expanded height remains remembered. |
| Standard | 232 px | Default Waveform height within the approved 220–240 px band. |
| Focused | 340 px | Preferred for Groups, Macros, Routing, Performance, and Articulations within the approved 320–360 px band. |

The active tab remains in the existing `WorkbenchState`, and the remembered height
lives in `WorkbenchLayoutState`. Both are members of one `AuthoringPanel`, so
they persist for the workspace UI session but have no project-document, host
recall, or authoring transaction path.

Tabs may suggest Standard or Focused height until the user directly resizes the
workbench. Once a user height exists, tab switching and zone-selection refreshes
retain it. Resizing a short host clamps only the allocated height; returning to
a larger host restores the remembered value.

## Splitter and keyboard contract

`WorkbenchSplitter` is a six-pixel boundary control with component ID
`authoringWorkbenchSplitter`.

- Dragging upward increases workbench height; dragging downward decreases it.
- Direct heights clamp to 220–360 px before host-space constraints are applied.
- Up/Down adjusts by 8 px; Shift+Up/Down adjusts by 32 px.
- Return or Space switches between Standard and Focused.
- Double-click provides the same Standard/Focused switch.
- The existing `authoringWorkbenchToggleButton` remains the keyboard-accessible
  collapse/expand action.

The splitter has explicit focus order 59, immediately before the workbench
toggle and tabs. Collapsing or switching a tab continues to move focus out of a
child that becomes hidden.

## Layout allocation

The workbench height is resolved from the remaining authoring area after the
summary, toolbar, and optional playback banner. The resolver subtracts the
explicit 160 px protected map height and the map/workbench gap before accepting
the preferred workbench height. The map and contextual mapping inspector then
consume all remaining center space through the existing expanded/compact shell
rules.

When a focused state cannot reach 320 px in a short plugin shell, the workbench
clamps below the focused band and its existing viewports retain access to the
full editor content. The remembered height is not changed by this clamp.

## Editor reflow

- Waveform uses a preview/metadata split at normal widths, keeping the waveform
  useful in Standard height without forcing Focused mode.
- Macros uses a first-class list/detail layout at demonstration size: creation,
  ordering, and selection remain in the left column; assignment and value detail
  remain in the right column. Short hosts retain vertical scrolling.
- Routing uses independent FX-chain and bus-routing columns in Focused expanded
  shells. Normal controls fit in the primary viewport with only a shallow path
  to diagnostic summaries; short and compact shells retain the established
  scroll-safe fallback.

Legacy `WorkbenchTab`, `WorkbenchState`, component IDs, and visible toggle text remain
intentionally stable for the migration pass. Accessibility titles now describe
the surface as the Authoring Workbench.

## Qualification

The dedicated `drs.open_workbench.phase3` target covers:

- pure state transitions, tab suggestions, user-height precedence, and host
  clamps;
- Standard, Focused, and Collapsed panel allocations;
- splitter pointer API and keyboard sizing without undo entries;
- macro list/detail and routing two-column geometry;
- active-tab and height retention across zone selection and host resize;
- compact/plugin tab parity and short-host map protection.

Phase 3 is also qualified against the existing Phase 2 authoring UI, mapping,
routing/macro, repeated-structure density, Phase 0–2 Open Workbench, and UI
responsiveness targets.
