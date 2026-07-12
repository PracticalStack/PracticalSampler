<!-- Diagram triage:
  - (no diagrams specified in Phase 1 data)
-->

# SliderPackData

SliderPackData is a scriptable data model for an array of discrete float values, used as the backing store for ScriptSliderPack UI components. It separates data from display - the values exist independently of any visual representation, and one or more ScriptSliderPack components can display the same data.

Create an instance with `Engine.createAndRegisterSliderPackData()`:

```js
const var spd = Engine.createAndRegisterSliderPackData(0);
```

The class supports three tiers of usage:

1. **Basic access** - read and write individual slider values, set the count and range.
2. **Undo-aware editing** - single-value and bulk edits that integrate with the undo manager, suitable for interactive step editing, recording, and pattern randomisation.
3. **Bulk data management** - preallocated buffers for variable-length sequencers, direct buffer access for efficient iteration, and Base64 serialisation for custom preset formats.

SliderPackData accepts the `[]` operator for reading and writing individual values (e.g. `spd[3] = 0.75`). By default, `[]` assignments bypass the undo system; call `setAssignIsUndoable(true)` to route them through the undo manager.

Two callback types are available: a content callback that fires when slider values change, and a display callback that fires when the ruler/playback position updates. For managing many SliderPackData objects, `Broadcaster.attachToComplexData("SliderPack.Content", ...)` can watch changes across all of them without per-object callbacks.

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

> [!Tip:Default value is 1.0, not zero] New SliderPackData objects default to 16 sliders in the range [0.0, 1.0] with a step size of 0.01. New sliders are filled with a default value of **1.0**, not 0.0. Call `setAllValues(0.0)` explicitly if you need zero-initialised steps.

## Common Mistakes

- **linkTo requires same complex data type**
  **Wrong:** `spd.linkTo(tableData)`
  **Right:** `spd.linkTo(otherSpd)`
  *`linkTo()` requires another SliderPackData object. Passing a different complex data type (Table, AudioFile) causes a type mismatch error.*

- **Use preallocation for fixed-size packs**
  **Wrong:** Creating SliderPackData objects without preallocation in a variable-length sequencer
  **Right:** Call `setUsePreallocatedLength(maxSteps)` immediately after creation
  *Without preallocation, every `setNumSliders()` call allocates a new buffer. With preallocation, resize adjusts a view into the fixed block with no allocation and no value loss.*

- **Use setValueWithUndo for user edits**
  **Wrong:** Using `setValue()` for user-initiated edits
  **Right:** Use `setValueWithUndo()` for interactive edits
  *`setValue()` cannot be undone. Users expect Ctrl+Z to work after editing steps, recording, or randomising patterns.*

- **Default slider value is 1.0 not 0.0**
  **Wrong:** Assuming new sliders initialise to `0.0`
  **Right:** New sliders initialise to `1.0` (the internal default)
  *Call `setAllValues(0.0)` explicitly after `setNumSliders()` if zero-initialisation is needed (e.g. empty sequencer steps).*
