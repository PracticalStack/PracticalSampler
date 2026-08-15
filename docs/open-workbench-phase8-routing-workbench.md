# Open Workbench Phase 8: Routing authoring controls

Date: August 14, 2026  
Status: implementation and automated qualification complete

## Delivered

- Replaced the Routing workbench's interleaved long form with a dedicated `RoutingWorkbenchView` organized around two coordinated regions: Bus & Signal Path, then Selected Insert.
- Added an explicit ordered signal-path strip from input through authored inserts to output. Bus input and Insert A/B selectors remain directly editable beside that presentation.
- Kept the DSP scope selector and canonical scope breadcrumb visible in wide, normal, compact, empty-chain, and short-host states.
- Added selected-insert context that exposes owner bus, insert position, and active/bypassed state without relying on diagnostics.
- Grouped selected-insert name, type, bypass, owner, and move-owner action under Identity, Owner & Actions.
- Grouped parameter selector, current value/unit/default, continuous control, Reset, and Create/Edit Control under Parameter & Macro Control.
- Added explicit Macro-control assignment status using the same assignment vocabulary introduced in Phase 7.
- Kept Add, Duplicate, Move Up, and Move Down together while separating Delete with the shared destructive semantic color.
- Placed concise FX state and diagnostics at the end of selected-insert detail. Legacy/unavailable and over-budget states use the shared warning role and an adjacent warning marker; healthy routing keeps the treatment quiet.
- Made the complete existing FX workflow available in compact mode. Compact and short hosts use the existing vertical viewport instead of hiding advanced operations.
- Extracted routing geometry from `AuthoringPanel` into a testable component with a read-only layout snapshot.

## Responsive contract

| State | Implemented composition |
| --- | --- |
| Wide Focused workbench | Bus/signal-path region followed by a wider selected-insert detail region. Primary path, owner, bypass, parameter, value, and Macro status remain in the initial viewport; secondary actions and diagnostics have bounded overflow. |
| Normal width | Balanced horizontal regions with the same workflow order and all operations retained. |
| Compact or short host | Bus/signal path followed by selected-insert detail in one predictable vertical sequence within the existing Routing viewport. |
| Empty bus graph | Scope and breadcrumb remain visible; an empty state points to Add Insert as the next valid action. |
| Empty selected chain | Add Insert remains available; identity and parameter operations are disabled while summary and diagnostics explain the state. |
| Warning state | Warning text and a narrow semantic marker appear adjacent to the selected-insert region without reserving a large diagnostics card. |

The layout preserves the 160 px protected map minimum and the existing remembered workbench-height behavior. It introduces no persistent UI state.

## Interaction and accessibility

- Focus order follows DSP scope, selected bus and insert order, selected FX identity/owner, parameter and Macro action, then secondary insert actions.
- The Routing viewport is described as a signal-path editor; accessibility help explains horizontal versus stacked composition.
- Signal path, owner, insert position, active/bypassed state, parameter value/default, Macro-control state, empty-state recovery, and warning state are expressed in text.
- All pre-existing externally exercised component IDs remain intact. New stable IDs cover signal-path heading/text, selected-FX identity/context, parameter heading, Macro status, and empty states.
- Tab changes and workbench collapse recognize focus within every newly reachable compact-mode FX control and return focus to the relevant tab or disclosure action.
- Destructive Delete uses the shared error role. Regions, typography, surfaces, borders, diagnostics, and warning state use the shared Open Workbench visual tokens.

## Behavior preserved

- Routing-bus selection, input editing, and the fixed two-insert model.
- FX create, duplicate, rename, type, bypass, reorder, move-owner, and delete transactions.
- Descriptor-driven parameter edit and Reset transactions.
- Create Control and Edit Control handoff into the existing Macro workflow.
- Existing transaction, revision, dirty-state, undo, redo, selection, and validation ownership.
- Existing graph-cost, preview, unavailable-effect, and tail-capability diagnostics.
- Standalone/VST3 model boundaries, host parameters, serialization, and DSP behavior.

No graph editor, node type, additional insert slot, metering, DSP behavior, host parameter, or serialized field was introduced.

## Qualification

`drs.open_workbench.phase8` covers:

- wide 1120×800 two-region geometry and workflow order;
- normal 820×700 balanced composition with every existing operation present;
- short 900×564 stacked composition and vertical-scroll fallback;
- preservation of the protected map minimum;
- explicit input-to-output path, owner, position, bypass, parameter, value, and Macro status;
- scope/path-to-FX-to-parameter-to-secondary-action focus order;
- destructive-action semantic color;
- bypass as one revision with undo and redo;
- parameter edit through the existing transaction path;
- no-bus and no-selected-FX recovery states; and
- legacy/unavailable warning treatment using the shared warning role.

Qualification results on Windows x64 with Visual Studio 2022 17.14.34:

| Gate | Result |
| --- | ---: |
| Phase 8 Debug | PASS |
| Phase 8 Release | PASS |
| Open Workbench Phases 0–8, authoring UI, macro/routing transactions, and repeated-density boundary | PASS — 12/12 |
| Debug standalone build | PASS |
| Release standalone build | PASS |
| Debug VST3 bundle compile/link | PASS |
| Release VST3 bundle compile/link | PASS |
| Source diff whitespace check and local-color audit | PASS |

The existing offscreen screenshot helper continues to produce blank images on this headless Windows runner. Visual qualification therefore used deterministic geometry, bounds, visibility, scrolling, hierarchy text, focus, and semantic-token assertions. The live standalone/VST3 cross-workspace demonstration remains part of Phase 9 convergence.

## Exit state

Phase 8 is complete. Routing now reads as an ordered signal path with scoped selected-insert detail, exposes the full existing workflow at compact sizes, and reuses Phase 7's Macro-assignment language without changing routing or DSP capabilities.
