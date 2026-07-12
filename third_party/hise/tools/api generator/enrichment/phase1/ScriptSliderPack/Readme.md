# ScriptSliderPack -- Class Analysis

## Brief
Multi-slider array UI component for editing SliderPackData with step and drag workflows.

## Purpose
ScriptSliderPack is the UI component created by `Content.addSliderPack()` for editing a vector of slider values. It specializes `ComplexDataScriptComponent` for `ExternalData::DataType::SliderPack`, so it can operate on internal data, processor-provided data slots, or externally referred `SliderPackData` objects. The class exposes bulk update methods, per-index writes, data-handle registration, and wrapper-driven interaction modes such as mouse-up-only callbacks, step-sequencer toggling, flash overlays, and custom slider width maps.

## Details

### Architecture Layers

- `ScriptComponent` provides common component properties, callback plumbing, and inherited API methods.
- `ComplexDataScriptComponent` provides external-data routing, source watching, base64 persistence of complex data, and `referToData`/registration infrastructure.
- `ScriptSliderPack` adds slider-pack property mapping and array-edit API methods.
- `SliderPackWrapper` maps properties to the JUCE `SliderPack` widget behavior.
- `SliderPackData` is the actual data model (`VariantBuffer`-backed float array with range, callbacks, and undo support).

### Data Source Modes

| Mode | Selected by | Data source |
|------|-------------|-------------|
| Internal | default | owned `SliderPackData` created in constructor |
| Processor slot | `processorId` + `SliderPackIndex` | connected processor `ExternalDataHolder` slot |
| Referred object | `referToData(...)` | holder from `ScriptSliderPackData` or another complex-data component |

### ScriptSliderPack-specific Property Effects

| Property | Runtime effect |
|----------|----------------|
| `sliderAmount` | updates slider count via `set("sliderAmount", value)`; see `ScriptSliderPack.getNumSliders` and `ScriptSliderPack.setWidthArray` |
| `stepSize` | updates slider data quantization step via `set("stepSize", value)` |
| `flashActive` | toggles flash overlay via `set("flashActive", value)` |
| `showValueOverlay` | toggles drag value overlay via `set("showValueOverlay", value)` |
| `SliderPackIndex` | selects bound external data slot via `set("SliderPackIndex", value)`; see `ScriptSliderPack.registerAtParent` |
| `mouseUpCallback` | defers callback/content-change emission to mouse-up commit |
| `stepSequencerMode` | switches drag model to step-sequencer behavior |

### Behavioral Selectors

- Callback behavior for bulk edits is controlled by `ScriptSliderPack.setAllValueChangeCausesCallback`; this affects `ScriptSliderPack.setSliderAtIndex` and `ScriptSliderPack.setAllValues`.
- Preallocated backing storage is controlled by `ScriptSliderPack.setUsePreallocatedLength`.
- Non-uniform width maps and hit-testing are controlled by `ScriptSliderPack.setWidthArray`.

## obtainedVia
`Content.addSliderPack(componentId, x, y)`

## minimalObjectToken
spk

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `spk.referToData(tableDataObj);` | `spk.referToData(sliderPackDataObj);` | `referToData` enforces matching complex-data type. Non-slider-pack objects trigger `Data Type mismatch` and do not bind. |
| `spk.setWidthArray([0.0, 0.5, 1.0]);` when `spk.getNumSliders()` is not `2` | Build the width array with `numSliders + 1` entries | Width maps are cumulative breakpoints and must be one element longer than slider count. Wrong length logs an error and produces invalid mapping assumptions. |
| Expecting `spk.setAllValuesWithUndo(...)` to stay callback-silent after `spk.setAllValueChangeCausesCallback(false)` | Treat undo bulk writes as callback-producing operations | Undo bulk writes always send content-change notification in current implementation, so callback suppression assumption is incorrect for this path. |

## codeExample
```javascript
const var spk = Content.addSliderPack("Steps", 10, 10);
spk.setAllValues([1.0, 0.5, 0.0, 0.75]);
spk.set("stepSequencerMode", true);
```

## Alternatives
- `ScriptSlider` -- use for one scalar value instead of an indexed value array.
- `ScriptTable` -- use for interpolated curve editing instead of discrete bar steps.
- `SliderPackData` -- use as a non-UI data handle for script-side array operations and linking.

## Related Preprocessors
`USE_BACKEND`, `HISE_NO_GUI_TOOLS`

## Diagrams

### complex-sliderpack-data-chain
- **Brief:** Slider Pack Data Chain
- **Type:** topology
- **Description:** Slider-pack workflows use a three-part chain. `SliderPackProcessor` selects the processor that owns one or more slider-pack slots, `SliderPackData` exposes the complex data stored in a specific slot, and `ScriptSliderPack` displays or edits that same slot in the UI. The binding pair is `processorId` plus `SliderPackIndex`, which is a complex-data connection and not the normal `parameterId` parameter binding path.

## Diagnostic Ideas
Reviewed: Yes
Count: 2
- ScriptSliderPack.setKeyPressCallback -- timeline-dependency (logged)
- ScriptSliderPack.setWidthArray -- state-validation (logged)
