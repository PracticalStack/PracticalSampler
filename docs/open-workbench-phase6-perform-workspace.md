# Open Workbench Phase 6: Perform workspace coherence

Date: August 14, 2026  
Status: implementation and automated qualification complete

## Delivered

- Replaced the Perform workspace's local dark palette and 18–20 px decorative cards with the shared Open Workbench shell, surface, border, type, focus, and semantic-state tokens.
- Added a compact identity header with instrument name, preset/articulation/playable-range context, a text-bearing readiness chip, recovery guidance, and an explicit `Details` disclosure.
- Rebalanced the workspace around published Instrument Controls. Expanded wide layouts place the control deck beside supporting artwork and give controls the larger share; compact and short layouts use a non-overlapping vertical stack.
- Kept the playable keyboard anchored at the bottom at target, compact, and short sizes. Playable range, note callbacks, and the existing audio-thread boundary are unchanged.
- Added explicit audio-unavailable presentation. The readiness chip says `Audio Inactive`, guidance points to audio settings/host processing, and the keyboard becomes visibly and accessibly unavailable.
- Restyled `PerformanceMixer` lanes with dense metrics, explicit name/value text, semantic control color, 3 px structural geometry, and blue focus independent of control value.
- Kept value-only mixer refreshes structural-stable; lane topology changes only when published control identity or kind changes.
- Moved the developer/runtime status surface behind `Details`, restyled it with the light token system, and hosted it in a vertical viewport so the secondary content remains usable without taking over normal Perform operation.

## Responsive contract

| State | Implemented composition |
| --- | --- |
| Wide, expanded controls | 68 px identity header; control/artwork split with approximately 58% of content width assigned to controls; bottom keyboard. |
| Normal or collapsed controls | Header; compact disclosure strip; supporting artwork; bottom keyboard. |
| Compact or short host | 64 px header; stacked controls and artwork; 84 px minimum keyboard; no overlap. |
| Details disclosed | A bounded, vertically scrollable diagnostics region appears above the keyboard while the primary regions reflow. |
| No active audio callback | Text-bearing warning state, recovery guidance, and disabled keyboard with an accessible explanation. |

The implementation exposes a read-only `PerformancePanel::LayoutSnapshot` for deterministic geometry qualification. It does not introduce persistent UI state, engine commands, host parameters, or serialized fields. The existing provider/callback path continues to remember the user's Instrument Controls expansion choice at the shell level.

## Accessibility and state treatment

- `Details` has focus order 10, Instrument Controls disclosure has focus order 20, legacy controls begin at 100, published controls begin at 200, and the keyboard remains at 500.
- Buttons and sliders use the shared blue focus ring. Focus returns to a disclosure action when its focused descendants are collapsed.
- Instrument identity, context, readiness, recovery guidance, hidden-control accounting, no-control state, and keyboard availability are expressed in text and accessible descriptions; color is supplementary.
- Ready, failed/missing, preview-degraded, and audio-unavailable states use shared success, error, warning, and information roles with low-chroma backgrounds and outlines.

## Behavior preserved

- Published macro/host binding identity and authored order.
- Macro and mixer value callbacks.
- Revision-aware structural and value-only refresh paths.
- Artwork payload/file fallback and explicit artwork refresh.
- MIDI keyboard range synchronization and note-on/note-off callbacks.
- Standalone/VST3 audio-callback provider boundaries and host activation semantics.

## Qualification

`drs.open_workbench.phase6` covers:

- target 1120×800 and compact/short 760×620 geometry;
- control-priority wide split, supporting artwork, and anchored keyboard;
- identity/context/readiness accessibility;
- Details disclosure and scroll-safe diagnostics;
- shared light shell rendering and semantic status tokens;
- explicit audio-unavailable guidance and keyboard state;
- stable published-control topology across value-only refreshes; and
- deterministic published-control focus order.

Qualification results on Windows x64 with Visual Studio 2022 17.14.34:

| Gate | Result |
| --- | ---: |
| Phase 6 Debug | PASS |
| Phase 6 Release | PASS |
| Existing Performance UI, PerformanceMixer, and Performance responsiveness | PASS |
| Open Workbench Phases 0–6, authoring UI, and repeated-density boundary | PASS |
| Debug standalone build | PASS |
| Release standalone build | PASS |
| Debug VST3 compile/link | PASS; the compatibility-copy post-step was blocked by an already-running REAPER process, which was intentionally left untouched. The source bundle contains the new binary. |

An attempted broader run reached the pre-existing `drs.phase2.performance_bank` test and made no progress for several minutes, so that aggregate invocation was stopped and the remaining 12 UI tests were rerun successfully. The Phase 6-focused suites and shell builds do not depend on that stalled case.

## Exit state

Phase 6 is complete. Perform now uses the same light, dense, square-edged control language as Map without changing performance behavior. Phase 7 can reuse this control language for the Macro authoring workbench.
