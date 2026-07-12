# SliderPackData -- Method Documentation

## fromBase64

**Signature:** `void fromBase64(String b64)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new VariantBuffer and swaps the internal data buffer.
**Minimal Example:** `{obj}.fromBase64("AAAAAAAAAIA/");`

**Description:**
Restores slider pack data from a Base64-encoded string containing raw float data. The number of sliders is determined by the decoded data size -- the slider count changes to match. This replaces the current buffer entirely.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| b64 | String | yes | Base64-encoded float data | Non-empty string |

**Pitfalls:**
- An empty string is silently ignored -- the data remains unchanged with no error.

**Cross References:**
- `$API.SliderPackData.toBase64$`
- `$API.SliderPackData.getNumSliders$`

## getCurrentlyDisplayedIndex

**Signature:** `float getCurrentlyDisplayedIndex()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var idx = {obj}.getCurrentlyDisplayedIndex();`

**Description:**
Returns the last display index value from the internal updater. This reflects the position of the ruler/playback indicator in a ScriptSliderPack UI component. Returns 0.0 if no display index has been set.

**Parameters:**
None.

**Cross References:**
- `$API.SliderPackData.setDisplayCallback$`

## getDataAsBuffer

**Signature:** `var getDataAsBuffer()`
**Return Type:** `Buffer`
**Call Scope:** safe
**Call Scope Note:** Returns a reference wrapper around the existing internal buffer without allocation.
**Minimal Example:** `var buf = {obj}.getDataAsBuffer();`

**Description:**
Returns the internal float data as a Buffer object. The returned Buffer is a direct reference to the underlying data -- not a copy. Writing to the Buffer modifies the slider pack values directly. This enables efficient DSP-style processing of slider values using Buffer operations.

**Parameters:**
None.

**Pitfalls:**
- The returned Buffer is a live reference. Any modification (e.g., `buf[0] = 0.5`) directly changes the slider pack data without triggering change notifications. Use `setValue()` or `setAllValues()` if you need listeners to be notified.

**Cross References:**
- `$API.SliderPackData.setAllValues$`
- `$API.SliderPackData.getValue$`

**Example:**
```javascript:buffer-reference-processing
// Title: Processing slider pack data with Buffer operations
const var spd = Engine.createAndRegisterSliderPackData(0);
spd.setNumSliders(8);
spd.setAllValues(0.5);

// Get the buffer reference and modify directly
var buf = spd.getDataAsBuffer();
buf[0] = 1.0;

// The slider pack data is now modified
Console.print(spd.getValue(0));
```
```json:testMetadata:buffer-reference-processing
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "spd.getValue(0)", "value": 1.0},
    {"type": "REPL", "expression": "spd.getValue(1)", "value": 0.5}
  ]
}
```

## getNumSliders

**Signature:** `int getNumSliders()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Acquires a lightweight read lock (atomic) to read the buffer size.
**Minimal Example:** `var num = {obj}.getNumSliders();`

**Description:**
Returns the number of sliders in the pack. Default is 16 if not changed via `setNumSliders()`.

**Parameters:**
None.

**Cross References:**
- `$API.SliderPackData.setNumSliders$`

## getStepSize

**Disabled:** no-op
**Disabled Reason:** Method is declared and implemented in C++ but not registered in the constructor (missing ADD_API_METHOD_0). Cannot be called from HiseScript. The step size can be set via setRange() but there is no script-accessible getter.

## getValue

**Signature:** `float getValue(int index)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Acquires a lightweight read lock (atomic) for bounds check and sample read.
**Minimal Example:** `var val = {obj}.getValue(0);`

**Description:**
Returns the slider value at the given index. If the index is out of range (negative or >= getNumSliders()), returns the default value (1.0) instead of throwing an error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Zero-based slider index | 0 to getNumSliders()-1 |

**Pitfalls:**
- Out-of-range indices silently return the default value (1.0) instead of throwing an error. This can mask off-by-one bugs since you get a plausible float value rather than an error.

**Cross References:**
- `$API.SliderPackData.setValue$`
- `$API.SliderPackData.getDataAsBuffer$`

## linkTo

**Signature:** `void linkTo(var other)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies event listener registrations and re-resolves the internal data pointer.
**Minimal Example:** `{obj}.linkTo(otherSpd);`

**Description:**
Links this SliderPackData to another SliderPackData so they share the same underlying data buffer. After linking, changes through either handle affect the shared data. Both objects must be SliderPackData type -- passing a different complex data type (Table, AudioFile) causes a "Type mismatch" script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| other | ScriptObject | no | Another SliderPackData to link to | Must be SliderPackData type |

**Cross References:**
- `$API.SliderPackData.getDataAsBuffer$`

**Example:**
```javascript:link-two-slider-packs
// Title: Linking two SliderPackData objects to share data
const var spd1 = Engine.createAndRegisterSliderPackData(0);
const var spd2 = Engine.createAndRegisterSliderPackData(1);

spd1.setNumSliders(4);
spd1.setAllValues(0.5);

// Link spd2 to spd1 -- they now share data
spd2.linkTo(spd1);

// Changes through spd1 are visible through spd2
spd1.setValue(0, 1.0);
Console.print(spd2.getValue(0));
```
```json:testMetadata:link-two-slider-packs
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "spd2.getValue(0)", "value": 1.0},
    {"type": "REPL", "expression": "spd2.getNumSliders()", "value": 4}
  ]
}
```

## setAllValues

**Signature:** `void setAllValues(var value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates an Array<float> and copies data into the internal buffer.
**Minimal Example:** `{obj}.setAllValues(0.5);`

**Description:**
Sets slider values from a single number, an Array, or a Buffer. When passed a single number, all sliders are set to that value. When passed an Array or Buffer, slider values are set from the elements -- only the first N sliders are updated where N is the source length. Values beyond the source length are not changed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | A Number (sets all), Array, or Buffer | -- |

**Pitfalls:**
- When passing an Array or Buffer shorter than the slider count, only the first N sliders are updated. The remaining sliders keep their previous values, not the default.

**Cross References:**
- `$API.SliderPackData.setAllValuesWithUndo$`
- `$API.SliderPackData.setValue$`

**Example:**
```javascript:set-all-values-polymorphic
// Title: Setting values from a number and an array
const var spd = Engine.createAndRegisterSliderPackData(0);
spd.setNumSliders(4);

// Set all sliders to 0.5
spd.setAllValues(0.5);

// Set from an array
spd.setAllValues([0.1, 0.2, 0.3, 0.4]);
Console.print(spd.getValue(2));
```
```json:testMetadata:set-all-values-polymorphic
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "spd.getValue(0)", "value": 0.1},
    {"type": "REPL", "expression": "spd.getValue(2)", "value": 0.3}
  ]
}
```

## setAllValuesWithUndo

**Signature:** `void setAllValuesWithUndo(var value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates an Array<float> and interacts with the undo manager.
**Minimal Example:** `{obj}.setAllValuesWithUndo(0.5);`

**Description:**
Same as `setAllValues()` but registers the operation with the undo manager, enabling undo/redo. Accepts a single number, Array, or Buffer. The undo action stores both old and new value arrays for complete restoration.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | A Number (sets all), Array, or Buffer | -- |

**Cross References:**
- `$API.SliderPackData.setAllValues$`
- `$API.SliderPackData.setValueWithUndo$`

## setAssignIsUndoable

**Signature:** `void setAssignIsUndoable(bool shouldBeUndoable)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets an internal boolean flag without any allocations or locks.
**Minimal Example:** `{obj}.setAssignIsUndoable(true);`

**Description:**
When enabled, `[]` operator assignments (e.g., `spd[3] = 0.75`) go through the undo system via `setValueWithUndo()` instead of `setValue()`. Disabled by default.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeUndoable | Integer | no | Whether [] assignments should be undoable | true/false |

**Cross References:**
- `$API.SliderPackData.setValueWithUndo$`
- `$API.SliderPackData.setValue$`

## setContentCallback

**Signature:** `void setContentCallback(var contentFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a WeakCallbackHolder for the callback function.
**Minimal Example:** `{obj}.setContentCallback(onContentChanged);`

**Description:**
Registers a callback that fires when slider values change. The callback receives a single integer argument: the index of the changed slider, or -1 for bulk operations (e.g., `setAllValues()`, `fromBase64()`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| contentFunction | Function | yes | Callback receiving changed slider index | 1 argument |

**Callback Signature:** contentFunction(sliderIndex: int)

**Cross References:**
- `$API.SliderPackData.setDisplayCallback$`
- `$API.SliderPackData.setValue$`

**Example:**
```javascript:content-callback-setup
// Title: Registering a content change callback
const var spd = Engine.createAndRegisterSliderPackData(0);
spd.setNumSliders(4);
spd.setAllValues(0.5);

reg lastChangedIndex = -99;

inline function onContentChanged(index)
{
    lastChangedIndex = index;
};

spd.setContentCallback(onContentChanged);

// --- test-only ---
spd.setValue(2, 0.8);
// --- end test-only ---
```
```json:testMetadata:content-callback-setup
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lastChangedIndex", "value": 2}
  ]
}
```

## setDisplayCallback

**Signature:** `void setDisplayCallback(var displayFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a WeakCallbackHolder for the callback function.
**Minimal Example:** `{obj}.setDisplayCallback(onDisplayChanged);`

**Description:**
Registers a callback that fires when the display/ruler position changes. The callback receives a single float argument: the display index position. This is typically driven by a ScriptSliderPack UI component's playback indicator or by calling `setPosition()` on the underlying data.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| displayFunction | Function | yes | Callback receiving display index | 1 argument |

**Callback Signature:** displayFunction(displayIndex: float)

**Cross References:**
- `$API.SliderPackData.getCurrentlyDisplayedIndex$`
- `$API.SliderPackData.setContentCallback$`

**Example:**
```javascript:display-callback-setup
// Title: Registering a display index callback
const var spd = Engine.createAndRegisterSliderPackData(0);

reg lastDisplayIndex = -1.0;

inline function onDisplayChanged(index)
{
    lastDisplayIndex = index;
};

spd.setDisplayCallback(onDisplayChanged);
```
```json:testMetadata:display-callback-setup
{
  "testable": false,
  "skipReason": "Display index changes require UI interaction or module playback to trigger"
}
```

## setNumSliders

**Signature:** `void setNumSliders(var numSliders)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new VariantBuffer when no preallocated length is set.
**Minimal Example:** `{obj}.setNumSliders(8);`

**Description:**
Sets the number of sliders. Existing values are preserved up to the new count. New sliders are filled with the default value (1.0). If a preallocated length is set via `setUsePreallocatedLength()`, resizing up to that limit adjusts the view without allocating, preserving all values. Values <= 0 are silently ignored.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numSliders | Number | no | Number of sliders | Must be > 0 |

**Pitfalls:**
- Values <= 0 are silently ignored -- the slider count remains unchanged with no error.

**Cross References:**
- `$API.SliderPackData.getNumSliders$`
- `$API.SliderPackData.setUsePreallocatedLength$`

## setRange

**Signature:** `void setRange(double minValue, double maxValue, double stepSize)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets two member variables with no locks or allocations.
**Minimal Example:** `{obj}.setRange(0.0, 1.0, 0.1);`

**Description:**
Sets the value range and step size for all sliders. The range constrains values in the UI (ScriptSliderPack component) and the step size controls value quantization. Default range is [0.0, 1.0] with step size 0.01. This does not clamp existing values -- only new values entered through the UI are constrained.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| minValue | Double | no | Minimum slider value | -- |
| maxValue | Double | no | Maximum slider value | Must be > minValue |
| stepSize | Double | no | Value quantization step | > 0 |

**Cross References:**
- `$API.SliderPackData.getValue$`
- `$API.SliderPackData.setValue$`

## setUsePreallocatedLength

**Signature:** `void setUsePreallocatedLength(int length)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a HeapBlock for the preallocated buffer.
**Minimal Example:** `{obj}.setUsePreallocatedLength(32);`

**Description:**
Configures a fixed-size preallocated memory block for the slider data. Once set, subsequent `setNumSliders()` calls up to the preallocated limit adjust the view into this fixed block without allocating new memory, preserving all existing values. Pass 0 to disable preallocation and return to normal allocation behavior. Useful for step sequencers where the step count changes frequently.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| length | Integer | no | Maximum number of sliders to preallocate for, or 0 to disable | >= 0 |

**Cross References:**
- `$API.SliderPackData.setNumSliders$`

**Example:**
```javascript:preallocated-resize
// Title: Using preallocated length for efficient resizing
const var spd = Engine.createAndRegisterSliderPackData(0);

// Preallocate for up to 32 sliders
spd.setUsePreallocatedLength(32);

// Set initial values
spd.setNumSliders(8);
spd.setAllValues(0.5);
spd.setValue(0, 1.0);

// Resize preserves existing values (no reallocation)
spd.setNumSliders(16);
Console.print(spd.getValue(0));
```
```json:testMetadata:preallocated-resize
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "spd.getValue(0)", "value": 1.0},
    {"type": "REPL", "expression": "spd.getNumSliders()", "value": 16}
  ]
}
```

## setValue

**Signature:** `void setValue(int sliderIndex, float value)`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** Acquires a read lock (atomic compare-exchange) and dispatches a synchronous content change notification.
**Minimal Example:** `{obj}.setValue(0, 0.75);`

**Description:**
Sets the value of a single slider at the given index. Out-of-range indices are silently ignored (bounds-checked via `isPositiveAndBelow`). The value is sanitized for non-finite numbers (NaN, infinity) before storage.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sliderIndex | Integer | no | Zero-based slider index | 0 to getNumSliders()-1 |
| value | Double | no | New slider value | Should be within range |

**Pitfalls:**
- Out-of-range indices are silently ignored with no error, which can mask off-by-one bugs.

**Cross References:**
- `$API.SliderPackData.getValue$`
- `$API.SliderPackData.setValueWithUndo$`

## setValueWithUndo

**Signature:** `void setValueWithUndo(int sliderIndex, float value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a SliderPackAction object for the undo manager.
**Minimal Example:** `{obj}.setValueWithUndo(0, 0.75);`

**Description:**
Sets a single slider value with undo support. Behaves like `setValue()` but registers the change with the undo manager, storing both old and new values for restoration.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| sliderIndex | Integer | no | Zero-based slider index | 0 to getNumSliders()-1 |
| value | Double | no | New slider value | Should be within range |

**Cross References:**
- `$API.SliderPackData.setValue$`
- `$API.SliderPackData.setAllValuesWithUndo$`
- `$API.SliderPackData.setAssignIsUndoable$`

## toBase64

**Signature:** `String toBase64()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a MemoryBlock and constructs a String from the encoded data.
**Minimal Example:** `var encoded = {obj}.toBase64();`

**Description:**
Exports all slider values as a Base64-encoded string containing the raw float array. Use `fromBase64()` to restore the data later. Returns an empty string if the underlying data is invalid.

**Parameters:**
None.

**Cross References:**
- `$API.SliderPackData.fromBase64$`
