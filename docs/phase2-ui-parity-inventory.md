# Phase 2 UI Parity Inventory

Last updated: July 16, 2026

## Purpose

This checklist records where every current authoring control lives in the Sprint 2 shell. It is the reachability reference for `UI25-206` while the old editor-mode selector still temporarily hosts macro, routing, and performance content.

## Shell Regions

- Summary strip: selection identity, project revision status, Preview, Undo, Redo, and Mark Saved.
- Toolbar row: zone selector and temporary `Editor` mode selector.
- Center workspace: persistent zone map in every editor mode.
- Right inspector: temporary host for Mapping, Macros, Routing, and Performance editors until workbench migrations complete.
- Bottom workbench: persistent tab strip plus Waveform content on the Waveform tab. Macros, Routing, and Performance tabs are placeholders in Sprint 2 and do not yet own their editors.

## Global Reachability

| Control / Information | Component ID | Sprint 2 location | Notes |
| --- | --- | --- | --- |
| Selected project / zone summary title | `authoringSummaryStrip` | Summary strip | Always visible in both shells |
| Revision / dirty / undo / redo status | `authoringSummaryStrip` | Summary strip | Always visible in both shells |
| Preview selected zone | `authoringPreviewButton` | Summary strip | Always visible in both shells |
| Undo | `authoringUndoButton` | Summary strip | Always visible in both shells |
| Redo | `authoringRedoButton` | Summary strip | Always visible in both shells |
| Mark Saved | `authoringSaveButton` | Summary strip | Always visible in both shells |
| Zone selector | `authoringZoneSelector` | Toolbar row | Always visible in both shells |
| Temporary editor selector | `authoringModeSelector` | Toolbar row | Remains until workbench migrations complete |
| Persistent zone map | `authoringZoneMap` | Center workspace | Visible in all four editor modes |
| Workbench collapse / expand | `authoringWorkbenchToggleButton` | Workbench tab strip | Compact defaults closed, expanded defaults open |
| Waveform workbench tab | `authoringWorkbenchWaveformTab` | Workbench tab strip | Owns live waveform content in Sprint 2 |
| Macros workbench tab | `authoringWorkbenchMacrosTab` | Workbench tab strip | Placeholder host only in Sprint 2 |
| Routing workbench tab | `authoringWorkbenchRoutingTab` | Workbench tab strip | Placeholder host only in Sprint 2 |
| Performance workbench tab | `authoringWorkbenchPerformanceTab` | Workbench tab strip | Placeholder host only in Sprint 2 |

## Mapping Mode

| Control / Information | Component ID | Sprint 2 location | Notes |
| --- | --- | --- | --- |
| Root key | `authoringRootKeySlider` | Right inspector | Temporary mapping editor bridge |
| Key low | `authoringKeyLowSlider` | Right inspector | Temporary mapping editor bridge |
| Key high | `authoringKeyHighSlider` | Right inspector | Temporary mapping editor bridge |
| Velocity low | `authoringVelocityLowSlider` | Right inspector | Temporary mapping editor bridge |
| Velocity high | `authoringVelocityHighSlider` | Right inspector | Temporary mapping editor bridge |
| Gain | `authoringGainSlider` | Right inspector | Temporary mapping editor bridge |
| Pan | `authoringPanSlider` | Right inspector | Temporary mapping editor bridge |
| Loop enabled | `authoringLoopEnabledToggle` | Right inspector | Temporary mapping editor bridge |
| Restore Root Key | `authoringRestoreRootKeyButton` | Right inspector | Temporary mapping editor bridge |

## Waveform Detail

| Control / Information | Component ID | Sprint 2 location | Notes |
| --- | --- | --- | --- |
| Waveform preview | `authoringWaveformPreview` | Bottom workbench, Waveform tab | Live content in Sprint 2 |
| Waveform summary text | `waveformLabel` / `waveformInfoLabel` | Bottom workbench, Waveform tab | Shell-owned labels |
| Loop metadata | `loopInfoLabel` | Bottom workbench, Waveform tab | Shell-owned label |
| Import responsiveness | `importMetricsLabel` | Bottom workbench, Waveform tab | Shell-owned label |

## Macros Mode

| Control / Information | Component ID | Sprint 2 location | Notes |
| --- | --- | --- | --- |
| Macro selector | `authoringMacroSelector` | Right inspector | Temporary inspector host |
| Macro parameter assignment | `authoringMacroAssignmentSelector` | Right inspector | Temporary inspector host |
| Macro role | `authoringMacroRoleSelector` | Right inspector | Temporary inspector host |
| Macro default | `authoringMacroDefaultSlider` | Right inspector | Temporary inspector host |
| Macro min | `authoringMacroMinSlider` | Right inspector | Temporary inspector host |
| Macro max | `authoringMacroMaxSlider` | Right inspector | Temporary inspector host |
| Move macro up | `authoringMacroMoveUpButton` | Right inspector | Temporary inspector host |
| Move macro down | `authoringMacroMoveDownButton` | Right inspector | Temporary inspector host |
| Future Macros tab entry point | `authoringWorkbenchMacrosTab` | Workbench tab strip | Placeholder only in Sprint 2 |

## Routing Mode

| Control / Information | Component ID | Sprint 2 location | Notes |
| --- | --- | --- | --- |
| FX selector | `authoringFxSelector` | Right inspector | Temporary inspector host |
| FX type | `authoringFxTypeSelector` | Right inspector | Temporary inspector host |
| FX bypassed | `authoringFxBypassedToggle` | Right inspector | Temporary inspector host |
| Routing bus selector | `authoringRoutingSelector` | Right inspector | Temporary inspector host |
| Routing input | `authoringRoutingInputSelector` | Right inspector | Temporary inspector host |
| Insert A | `authoringRoutingInsertOneSelector` | Right inspector | Temporary inspector host |
| Insert B | `authoringRoutingInsertTwoSelector` | Right inspector | Temporary inspector host |
| Future Routing tab entry point | `authoringWorkbenchRoutingTab` | Workbench tab strip | Placeholder only in Sprint 2 |

## Performance Mode

| Control / Information | Component ID | Sprint 2 location | Notes |
| --- | --- | --- | --- |
| Performance bank selector | `authoringPerformanceBankSelector` | Right inspector | Temporary inspector host |
| Trigger slot selector | `authoringTriggerSlotSelector` | Right inspector | Temporary inspector host |
| Trigger event | `authoringTriggerEventSelector` | Right inspector | Temporary inspector host |
| Target articulation | `authoringTargetArticulationSelector` | Right inspector | Temporary inspector host |
| Phrase asset | `authoringPhraseAssetSelector` | Right inspector | Temporary inspector host |
| Chord rule | `authoringChordModeSelector` | Right inspector | Temporary inspector host |
| MIDI import path | `authoringPhraseImportPath` | Right inspector | Temporary inspector host |
| Import MIDI phrase | `authoringPhraseImportButton` | Right inspector | Temporary inspector host |
| Future Performance tab entry point | `authoringWorkbenchPerformanceTab` | Workbench tab strip | Placeholder only in Sprint 2 |

## Sprint 2 Notes

- There is one production editor per currently editable value in Sprint 2.
- No duplicate editable macro, routing, or performance controls were introduced into the workbench; those tabs are placeholders until Sprint 4 migrations land.
- The mode selector is still the source of truth for those three editor families during Sprint 2.
