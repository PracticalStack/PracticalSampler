# Open Workbench Phase 9: Pass 03 convergence and stop

Date: August 14, 2026  
Status: implementation and automated qualification complete

## Delivered

- Added `drs.open_workbench.phase9`, a focused convergence qualification that runs the real shared Perform and Map workspaces through standalone and plug-in shell implementations.
- Added a deterministic Map → Perform → Macros → Routing → Map journey in both shells. It verifies zone selection, a published Perform value, three audible note gestures, Macro and Routing transactions, undo/redo, active workbench scope, remembered height, collapse, Fit All, and final state recovery.
- Covered standalone 1120×800 and 900×700; plug-in 900×700, 820×700, and 760×620; and 100%, 125%, 150%, and 200% scaled paint traversal.
- Audited Perform, Macro, and Routing paint paths. All in-scope color use resolves through `OpenWorkbenchVisualSystem.h`; structure remains 1px and geometry remains 2–3px. No alternate appearance path was found.
- Audited shared controls across Waveform, Groups, Performance authoring, Articulations, Map, and both shells. All retained protected map space and their existing tab/focus contracts.
- Added missing accessibility help text to the Perform Details and Instrument Controls disclosures.
- Added `tools/qualify-open-workbench-phase9.ps1` as the repeatable Debug/Release stop gate.
- Updated the developer extension guide with the Perform/Macro/Routing component boundaries and frozen Pass 03 conventions.

## Qualification hardening

Phase 9 exposed test debt rather than production regressions:

- `drs.sprint6.publish_shell_parity` and `drs.phase2.authoring_playback_integration` keep large processor-backed shells in test-main stack frames. They now use the same 8 MiB MSVC test stack as the existing shell-heavy gates. Production binaries are unchanged.
- The authoring-playback integration now follows the current bounded-preview contract: explicitly authorize the first preparation, cross the message/audio activation boundary, and explicitly select the newly imported zone before editing.
- The optimized Publish-controller concurrency test now waits for its reader thread to start before allowing the writer to finish all iterations.
- The qualification wrapper runs native tests in isolated invocations and permits one retry for a headless native teardown failure. A repeated failure remains blocking.

## Focused Phase 9 coverage

| Area | Evidence |
| --- | --- |
| Visual source | Perform, Macro, and Routing scrollbar, tooltip, menu, destructive, focus, border, radius, and surface assertions resolve to shared semantic roles. Source audit found no local color literals in the changed paint paths. |
| Shell matrix | Standalone and plug-in supported sizes retain reachable Perform/Map roots, keyboard, workbench, and at least 160px of Map. |
| Cross-workspace state | Published Perform value and selected zone survive Perform/Macro/Routing visits; Macro and Routing undo/redo preserve tab and workbench height. |
| Playback | Three performance note gestures produce audible output in each shell path; Phase 0 smoke and authoring-playback integration remain green. |
| Existing tabs | Waveform, Groups, Performance authoring, and Articulations remain reachable and do not consume protected Map height. |
| Accessibility | Changed interactive controls retain stable IDs, titles, descriptions, help text, focus order, and semantic disabled/destructive treatment. |

## Qualification results

Qualification machine: Windows x64, Visual Studio 2022 17.14.34.

| Gate | Result |
| --- | ---: |
| Phase 9 Debug | PASS — 7.41 s |
| Phase 9 Release | PASS — 4.66 s |
| Debug functional/UI matrix | PASS — 32 gates, including VST3 host state, playback, publish, Macro/Routing, density, authoring UI, Phases 0–9, and focused Perform responsiveness |
| Release functional/UI/responsiveness matrix | PASS — 33/33 gates |
| Release 1,700-zone responsiveness authority | PASS — 59.31 s |
| Debug focused Perform responsiveness | PASS — 2.05 s |
| Debug and Release standalone | PASS — compile/link |
| Debug and Release VST3 bundle | PASS — compile/link and host scan/state qualification |
| Source diff whitespace and local-color audit | PASS |

The full Release script completed without a retry. During cumulative Debug qualification, one Phase 5 headless teardown crashed after 46 seconds; its isolated retry passed in 4.59 seconds. This is classified as runner-only because Phase 5 passed before the cumulative run, on retry, and in Release, and no production path changed.

## Demonstration status

The deterministic native-shell demonstration passes in both shell implementations and the built VST3 passes host scan/state recall. The existing headless Windows renderer still cannot provide trustworthy pixel screenshots. Physical precision-trackpad behavior and visual inspection at native 125%, 150%, and 200% OS scaling in a production VST3 host remain operator follow-ups; they do not change the automated stop decision.

## Issue triage

- Release-blocking: none found.
- Deferred validation: physical trackpad gesture feel and live-host high-DPI visual inspection.
- Explicitly excluded: branding/logo/iconography, dark theme, library/browser/content discovery, new Macro or Routing capabilities, a graph editor, and a general UI sweep.

## Stop decision

Pass 03 is complete for the agreed product surface. Perform, Macros, Routing, Map, and the existing authoring shell now share one qualified light desktop-tool direction. Stop UI expansion here and return to functionality and demonstration work.
