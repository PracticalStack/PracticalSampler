# SliderPackProcessor -- Class Analysis

## Brief
Script handle to a module with SliderPack data, providing indexed data references.

## Purpose
SliderPackProcessor is a lightweight wrapper around any audio module that contains SliderPack data (step sequencers, array modulators, harmonic filters, etc.). Obtained via `Synth.getSliderPackProcessor()`, it serves as a bridge to access the module's SliderPack data objects by index. Its single method `getSliderPack()` returns a `SliderPackData` reference for reading and writing slider values. The wrapper accepts any module implementing the ExternalDataHolder interface, not just modules that inherit from the C++ SliderPackProcessor base class.

## obtainedVia
`Synth.getSliderPackProcessor(processorId)`

## minimalObjectToken
spp

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `var spp = Synth.getSliderPackProcessor("MyModule");` called in `onNoteOn` | `const var spp = Synth.getSliderPackProcessor("MyModule");` in `onInit` | Must be called in onInit -- the factory method requires the object creation phase and will throw an error in callbacks. |

## codeExample
```javascript
// Get a reference to a module's slider pack data
const var spp = Synth.getSliderPackProcessor("ArrayModulator1");
const var pack = spp.getSliderPack(0);
```

## Alternatives
TableProcessor (same pattern for Table data), AudioSampleProcessor (same pattern for audio file data), DisplayBufferSource (same pattern for display buffers).

## Related Preprocessors
None.

## Diagrams

### complex-sliderpack-data-chain
- **Brief:** Slider Pack Data Chain
- **Type:** topology
- **Description:** Slider-pack workflows use a three-part chain. `SliderPackProcessor` selects the processor that owns one or more slider-pack slots, `SliderPackData` exposes the complex data stored in a specific slot, and `ScriptSliderPack` displays or edits that same slot in the UI. The binding pair is `processorId` plus `SliderPackIndex`, which is a complex-data connection and not the normal `parameterId` parameter binding path.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Single method class with no mode selectors or silent-failure preconditions beyond the standard onInit restriction already covered by CHECK_MODULE diagnostics.
