# SliderPackData -- Class Analysis

## Brief
Scriptable handle to a discrete float value array for step sequencers, arpeggiators, and multi-slider data.

## Purpose
SliderPackData provides programmatic access to an array of float values that can be displayed and edited by a ScriptSliderPack UI component. It serves as the data model in a model-view separation -- the data exists independently of any visual representation. The class supports indexed read/write access (including `[]` operator syntax), bulk value operations, undo/redo, serialization to Base64, and event callbacks for both content changes and display index updates. It can also link to other SliderPackData instances to share the same underlying data.

## Details

### Data Storage and Defaults
The underlying storage is a `VariantBuffer` (float array). Default state:
- 16 sliders
- Range: [0.0, 1.0]
- Step size: 0.01
- Default fill value: 1.0 (not 0.0 -- new sliders are initialized to 1.0)

### Preallocated Length
By default, resizing with `setNumSliders()` allocates a new buffer and copies existing values, filling new slots with the default value (1.0). See `setUsePreallocatedLength()` for a fixed-allocation mode that avoids reallocation on resize.

### Undo Support
Three levels of undo integration: single-value (`setValueWithUndo()`), bulk (`setAllValuesWithUndo()`), and operator-level (`setAssignIsUndoable(true)` makes `[]` assignments undoable). See each method entry for details.

### Event Callbacks
Two callback types via the ComplexDataUIUpdater system: a display callback for ruler/position changes (`setDisplayCallback()`) and a content callback for value changes (`setContentCallback()`). See each method entry for callback signatures and argument details.

### Buffer Reference Semantics
`getDataAsBuffer()` returns the internal VariantBuffer by reference, not a copy. See the method entry for implications of direct buffer modification.

### Linking
`linkTo()` makes two SliderPackData handles share the same underlying data. See the method entry for type constraints and behavior.

### setAllValues Polymorphism
`setAllValues()` and `setAllValuesWithUndo()` accept a single number, an Array, or a Buffer. See the method entries for details on partial-update behavior.

## obtainedVia
`Engine.createAndRegisterSliderPackData(index)`

## minimalObjectToken
spd

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `spd.linkTo(tableData)` | `spd.linkTo(otherSpd)` | `linkTo()` requires another SliderPackData object. Passing a different complex data type (Table, AudioFile) causes a type mismatch error. |

## codeExample
```javascript
// Create a SliderPackData with 8 steps in range [0, 1]
const var spd = Engine.createAndRegisterSliderPackData(0);
spd.setNumSliders(8);
spd.setRange(0.0, 1.0, 0.1);
spd.setAllValues(0.5);

// Read and write individual values
spd.setValue(0, 1.0);
var firstVal = spd.getValue(0);

// Use [] operator syntax
spd[3] = 0.75;
var fourthVal = spd[3];

// Listen for changes
spd.setContentCallback(function(sliderIndex)
{
    Console.print("Slider " + sliderIndex + " changed");
});
```

## Alternatives
- **Table** -- Use for continuous curve data defined by control points; SliderPackData is for discrete slider values.
- **AudioFile** -- Use for audio waveform references; SliderPackData is for arrays of parameter values.
- **Buffer** -- Use for standalone float arrays you process directly; SliderPackData is for float arrays bound to a UI slider pack.
- **DisplayBuffer** -- Use for read-only ring buffer visualization; SliderPackData is for editable discrete values.
- **ScriptSliderPack** -- The UI component that displays and edits a SliderPackData object.

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
Rationale: SliderPackData methods have straightforward value semantics with no silent-failure preconditions or timeline dependencies that would benefit from parse-time diagnostics.
