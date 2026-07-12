<!-- Diagram triage:
  - none: SKIP (no class-level or method-level diagram specs in ScriptSliderPack JSON)
-->
# ScriptSliderPack

`ScriptSliderPack` is a multi-slider editor for array-based values, typically used for step lanes, rhythmic grids, and other indexed control data.

Use it when one UI component needs to edit many related values together instead of one scalar parameter. In practice, most workflows fall into three patterns:

- A single lane editor with one callback that reacts to the edited index.
- Shared-data editing where multiple views bind to the same slider-pack data handle.
- Sequencer-style editing with width maps, callback suppression during imports, and custom local look and feel.

Create it once in `onInit`, then configure slider count, data binding, and callback behaviour before runtime editing starts.

```javascript
const var spk = Content.addSliderPack("Steps", 10, 10);
spk.set("sliderAmount", 16);
spk.setAllValues([1.0, 0.5, 0.0, 0.75]);
```

`ScriptSliderPack` inherits the general `ScriptComponent` utility surface (layout, visibility, focus, tooltip, z-level, stylesheet helpers) and the complex-data binding workflow (`referToData`, `registerAtParent`, buffer access).

Use `setControlCallback` with index-centric logic: treat the callback `value` as the edited index, then read the actual lane value with `getSliderValueAt(index)`.

For bulk imports, temporarily disable callback fanout with `setAllValueChangeCausesCallback(0)`, write values, then run one explicit downstream refresh.

## Complex Data Chain

Slider-pack workflows use a three-part complex-data chain:

![Slider Pack Data Chain](topology_complex-sliderpack-data-chain.svg)

- `SliderPackProcessor` selects the module that owns one or more slider-pack slots.
- `SliderPackData` is the complex-data handle for one slot within that module.
- `ScriptSliderPack` displays or edits one selected slot in the UI.

Use the binding properties separately:

- `processorId` selects the owning processor.
- `SliderPackIndex` selects which slider-pack slot inside that processor should be displayed.

This is not the normal parameter binding path. `parameterId` targets processor parameters, while slider-pack binding uses `SliderPackIndex` instead.

> [!Tip:Array data class, scalar helpers inactive] This class edits slider-pack array data, not single scalar values. Inherited scalar helpers like `getValueNormalized`, `setValueNormalized`, `setValueWithUndo`, and `updateValueFromProcessorConnection` are intentionally not active here. For JSON property restore, use `Content.setPropertiesFromJSON(componentId, jsonData)`.

## Common Mistakes

- **Undo writes always notify callbacks**
  **Wrong:** Expecting `spk.setAllValuesWithUndo(...)` to stay callback-silent after `spk.setAllValueChangeCausesCallback(0)`
  **Right:** Treat `setAllValuesWithUndo(...)` as callback-producing and guard callback logic when needed
  *Undo bulk writes still emit change notification, so callback suppression assumptions do not apply to that path.*

- **Value is slider index not slider value**
  **Wrong:** Treating the `value` argument in `setControlCallback` as the lane amplitude
  **Right:** Parse it as the edited index, then call `getSliderValueAt(index)`
  *Slider-pack callbacks are index-centric in common lane editors, so direct use as value can read the wrong data.*

- **Disable callbacks during bulk import**
  **Wrong:** Importing large arrays with callbacks enabled
  **Right:** Disable callbacks for import, then trigger one explicit refresh after the write
  *This avoids repeated rebuild logic and unnecessary UI churn during setup operations.*

- **Update sliderAmount and widths together**
  **Wrong:** Updating `setWidthArray(...)` without updating `sliderAmount` in the same code path
  **Right:** Set slider count and width map together from one grid/subdivision function
  *Width breakpoints and slider count must stay aligned for predictable drawing and hit-testing.*
