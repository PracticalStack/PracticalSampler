<!-- Diagram triage:
  - No diagrams specified in Phase 1 data.
-->

# SliderPackProcessor

SliderPackProcessor is a lightweight wrapper around any audio module that contains SliderPack data - step sequencers, array modulators, harmonic filters, and similar modules with indexed slider arrays. Obtained via `Synth.getSliderPackProcessor()`, it provides access to the module's SliderPack data objects by index.

```js
const var spp = Synth.getSliderPackProcessor("ArrayModulator1");
const var pack = spp.getSliderPack(0);
```

The returned `SliderPackData` reference lets you read and write individual slider values, resize the pack, and perform other slider pack operations. This follows the same processor-wrapper pattern as `TableProcessor` for table data and `AudioSampleProcessor` for audio file data.

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

> [!Tip:Accepts any module with SliderPack data] The wrapper accepts any module that holds SliderPack data internally, not only modules whose name suggests slider pack functionality.

## Common Mistakes

- **Cache processor reference in onInit**
  **Wrong:** `var spp = Synth.getSliderPackProcessor("MyModule");` called in `onNoteOn`
  **Right:** `const var spp = Synth.getSliderPackProcessor("MyModule");` in `onInit`
  *The factory method requires the object creation phase and will throw an error if called inside a callback.*
