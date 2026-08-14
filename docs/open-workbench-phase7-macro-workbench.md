# Open Workbench Phase 7: Macro authoring controls

Date: August 14, 2026  
Status: implementation and automated qualification complete

## Delivered

- Replaced the Macro workbench's single cramped control form with a dedicated `MacroWorkbenchView` organized around an ordered Macro list, Macro definition, and assigned targets.
- Kept Create, Duplicate, Move Up, and Move Down adjacent to the ordered list. Delete remains separated and uses the shared destructive color role.
- Grouped Name, Role, and Expose to Host under Identity & Host. Grouped Default, Minimum, and Maximum under Range with explicit numeric status text.
- Added a scannable Assigned Targets list. Each row exposes target family, target name or path, and the current mapping summary; the selected target retains identity and mapping detail while it is edited.
- Added explicit Add Target and Remove Target actions using the existing curated target vocabulary and `AuthoringSession::updateMacro` transaction path. The Macro model, serialized representation, and host parameter topology are unchanged.
- Added purposeful states for no authored Macros, no selected Macro, and no assigned targets, including the next valid action.
- Preserved the existing public component IDs and added stable IDs for the target list, target actions, section headings, range status, and selected-target detail.
- Extracted responsive geometry from `AuthoringPanel` into a testable component with a read-only layout snapshot.

## Responsive contract

| State | Implemented composition |
| --- | --- |
| Wide Focused workbench | Three horizontal regions: ordered Macro list and actions; definition and range; assigned targets and selected-target detail. |
| Normal width | Macro list beside a balanced definition/assignment detail area; the complete editor remains above the fold at the standard 820×700 compact shell. |
| Compact or short host | One vertical list, definition, assignment sequence inside the existing Macro viewport; vertical scrolling appears when required. |
| No authored Macros | Create remains available; definition and assignment regions explain that a Macro must be created or selected. |
| Selected Macro without targets | Identity and range remain editable; target guidance and Add Target remain available while Remove Target is disabled. |

The layout component does not own project data. `AuthoringPanel` remains the coordinator for selection and callbacks, while all authored changes continue through existing session transactions.

## Interaction and accessibility

- Focus order follows Macro list, identity and range, assigned-target list and detail, then list actions.
- Macro selection remains stable across target edits and zone-selection refreshes. Target selection remains valid after add, remove, undo, redo, and project refresh.
- Add Target chooses the first supported target not already assigned in the current project context. When all curated targets are assigned, the existing information-dialog pattern explains why no target can be added.
- Assignment editing affects the selected target rather than implicitly rewriting the first target.
- Accessible descriptions explain target count, the selected target, action availability, range behavior, and the next valid empty-state action. Color supplements rather than replaces text.
- Destructive Delete and Remove Target actions use the shared error role; regions, typography, borders, and surfaces use the shared Open Workbench visual tokens.

## Behavior preserved

- Macro create, duplicate, rename, host exposure, reorder, and delete transaction paths.
- Existing curated generic and DSP target choices and role vocabulary.
- Existing range clamping and ordering semantics.
- Document revision, undo, redo, dirty-state, and selection ownership.
- Existing Macro component IDs used by UI automation.
- Standalone and VST3 state/model boundaries; no host parameters or serialization fields were added.

## Qualification

`drs.open_workbench.phase7` covers:

- wide 1120×800 three-region geometry and hierarchy;
- normal 820×700 list/detail geometry without unnecessary scrolling;
- short 900×564 stacked geometry with vertical scrolling;
- target-list completeness and selected-target detail;
- list-to-definition-to-assignment focus order;
- destructive-action semantic color;
- Add Target as one revision, with undo and redo;
- removal of only the selected assignment;
- Macro-selection stability across zone refreshes; and
- no-Macro and no-target action states.

Qualification results on Windows x64 with Visual Studio 2022 17.14.34:

| Gate | Result |
| --- | ---: |
| Phase 7 Debug | PASS |
| Phase 7 Release | PASS |
| Open Workbench Phases 0–7, authoring UI, macro/routing transactions, and repeated-density boundary | PASS — 11/11 |
| Debug standalone build | PASS |
| Release standalone build | PASS |
| Debug VST3 bundle compile/link | PASS |
| Release VST3 bundle compile/link | PASS |
| Source diff whitespace check and local-color audit | PASS |

The existing offscreen screenshot helper produced blank images on this headless Windows runner. Visual qualification therefore used deterministic geometry, visibility, scroll, focus, text hierarchy, and semantic-token assertions. A live-host demonstration pass remains appropriate during Phase 9 convergence.

## Exit state

Phase 7 is complete. Macros now reads as a definition-and-assignment editor rather than a compressed generic form, supports the full existing authored workflow at wide and compact sizes, and establishes the assignment presentation that Phase 8 Routing can reuse.
