# Open Workbench Phase 5 demonstration script

Use this script after `tools/qualify-open-workbench-phase5.ps1` passes. Record the shell, display scale, input device, instrument, result, and issue ID for every failure.

## Setup

- Standalone target: 1120 × 800, then 900 × 700.
- Plugin target: 820 × 700, then 760 × 620 where the host permits manual editor sizing.
- Display scales: 100%, 125%, 150%, and 200% across available qualification displays.
- Instruments: deterministic 642-zone fixture, deterministic 1,000-zone fixture, and the Accurate Salamander corpus when installed.
- Begin at Fit All with the workbench collapsed and Preview stopped.

## Mouse and trackpad navigation

1. Confirm the overview shows group structure without persistent zone-name clutter.
2. Click and drag the minimap viewport into a dense region. Selection must not change.
3. Point at a narrow zone and use stepped wheel zoom. The pointed zone must remain under the pointer.
4. Use middle-button drag to pan. Selection and active workbench tab must remain stable.
5. Hold Space and left-drag to pan, release Space, then drag empty map space to marquee-select. Gesture arbitration must switch back to selection.
6. On a precision trackpad, use a smooth two-axis gesture. It must pan naturally; Shift-scroll must pan only horizontally. Record `not available` rather than inferring a trackpad result on mouse-only hardware.

## Selection and editing

1. Select one zone, Control-click two separated zones, and run Fit Selected.
2. At Detail scale, drag a key-range handle and then a velocity-crossfade handle.
3. Undo once and redo once while still zoomed. Viewport, primary selection, and secondary selection must remain stable.
4. Drag a supported WAV or FLAC file onto the map. The drop overlay must be visible and import must use the existing asynchronous workflow.
5. Right-click a selected zone and verify the contextual delete action remains available without changing zoom.

## Preview and workbench

1. Preview the selected zone, stop it, then preview again.
2. Open Waveform at Standard height, collapse while a child has focus, and verify focus returns to the workbench rail.
3. Open Routing, expand to Focused height, edit one FX parameter and one bus field, then undo/redo.
4. Open Macros, edit an assignment, change zone selection, and confirm the active tab and remembered height remain stable.
5. Visit Groups, Performance, and Articulations. Confirm each tab has a visible title, scope, breadcrumb, and reachable first/last control.
6. Collapse the workbench, run Fit All, and confirm the map receives the recovered space.

## Keyboard-only pass

1. Tab from map controls through Fit All, Fit Selected, Zoom Out, Zoom In, the minimap, splitter, workbench toggle, and all six workbench tabs.
2. Use arrow keys on the map to change primary selection and on the minimap to pan the viewport.
3. Use Up/Down on the splitter and Return to switch between Standard and Focused height.
4. Activate every workbench tab with Space or Return and verify visible blue focus remains independent of orange selection.
5. Collapse the workbench while focus is inside Macros, Routing, and Articulations. Focus must never remain in hidden content.

## Shell and scaling pass

Repeat the Fit All, Fit Selected, minimap, workbench collapse/expand, and keyboard focus checks in standalone and VST3 shells at every available display scale. There must be no clipped controls, doubled panel borders, unreadable metadata, or focus indicators outside component bounds.

## Completion record

The pass is complete only when automated qualification is green and manual results are recorded. Classify findings as:

- `release-blocking`: prevents a reliable demonstration or corrupts authored/playback state;
- `follow-up`: real usability/accessibility issue that does not block the approved demonstration;
- `production-UI backlog`: branding, iconography, optional theme variants, or polish outside this iteration.
