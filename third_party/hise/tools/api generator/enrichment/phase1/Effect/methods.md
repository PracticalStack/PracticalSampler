## exists

**Signature:** `bool exists()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var valid = {obj}.exists();`

**Description:**
Returns whether the effect module referenced by this handle still exists in the module tree. Returns false if the underlying processor has been deleted (e.g., after dynamic removal via Builder). Internally calls the base class `checkValidObject()` which tests the weak reference validity.

**Parameters:**
None.

## getAttribute

**Signature:** `float getAttribute(int index)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var value = {obj}.getAttribute({obj}.Frequency);`

**Description:**
Returns the current value of the parameter at the given index. Use the effect's named constants for readable index access (e.g., `fx.Frequency` instead of a raw integer).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Number | yes | The parameter index | 0 to getNumAttributes()-1 |

**Cross References:**
- `$API.Effect.setAttribute$`
- `$API.Effect.getAttributeId$`

## getAttributeId

**Signature:** `String getAttributeId(int index)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return involves atomic ref-count operations.
**Minimal Example:** `var name = {obj}.getAttributeId(0);`

**Description:**
Returns the name of the parameter at the given index as a string. Useful for building dynamic UIs or debugging parameter mappings.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Number | yes | The parameter index | 0 to getNumAttributes()-1 |

**Cross References:**
- `$API.Effect.getAttributeIndex$`
- `$API.Effect.getNumAttributes$`

## getAttributeIndex

**Signature:** `int getAttributeIndex(String id)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String parameter comparison involves atomic ref-count operations.
**Minimal Example:** `var idx = {obj}.getAttributeIndex("Frequency");`

**Description:**
Returns the parameter index for the given parameter name string. Returns -1 if no parameter with the given name exists.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | yes | The parameter name | Must match a valid parameter name |

**Cross References:**
- `$API.Effect.getAttributeId$`

## getId

**Signature:** `String getId()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return involves atomic ref-count operations.
**Minimal Example:** `var id = {obj}.getId();`

**Description:**
Returns the module ID of the wrapped effect processor as a string.

**Parameters:**
None.

## getNumAttributes

**Signature:** `int getNumAttributes()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var count = {obj}.getNumAttributes();`

**Description:**
Returns the total number of parameters exposed by the wrapped effect module. This count matches the number of named constants available on the Effect handle.

**Parameters:**
None.

**Cross References:**
- `$API.Effect.getAttribute$`
- `$API.Effect.getAttributeId$`

## isBypassed

**Signature:** `bool isBypassed()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var bypassed = {obj}.isBypassed();`

**Description:**
Returns whether the effect is currently bypassed.

**Parameters:**
None.

**Cross References:**
- `$API.Effect.setBypassed$`

## isSuspended

**Signature:** `bool isSuspended()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var suspended = {obj}.isSuspended();`

**Description:**
Returns whether the effect is currently suspended due to silence detection. Returns true only when both conditions are met: the effect has opted into silence suspension internally, AND the effect is currently in suspended state (after approximately 86 silent audio callbacks). Effects that do not implement silence suspension always return false.

**Parameters:**
None.

**Pitfalls:**
- Always returns false for effects that have not opted into silence suspension via their internal `isSuspendedOnSilence()` flag, regardless of whether audio is actually flowing. This opt-in is not configurable from script -- it is a property of the effect type.

## setAttribute

**Signature:** `void setAttribute(int parameterIndex, float newValue)`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** Uses context-dependent notification via getAttributeNotificationType(). On the audio thread, sets the value with dontSendNotification. On other threads, involves ValueTree property update with string lookup and notification dispatch.
**Minimal Example:** `{obj}.setAttribute({obj}.Frequency, 1000.0);`

**Description:**
Sets the value of the parameter at the given index. Use the effect's named constants for readable index access. The notification type adapts automatically to the calling thread context, making this method safe to call from any thread.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Number | yes | The parameter index | 0 to getNumAttributes()-1 |
| newValue | Number | yes | The new parameter value | Parameter-dependent range |

**Cross References:**
- `$API.Effect.getAttribute$`
- `$API.Effect.getAttributeId$`
- `$API.Effect.getAttributeIndex$`

## setBypassed

**Signature:** `void setBypassed(bool shouldBeBypassed)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Sends bypass notification and an async dispatch message for UI update.
**Minimal Example:** `{obj}.setBypassed(true);`

**Description:**
Enables or disables bypass for the effect. When bypassed, the effect passes audio through without processing. MasterEffectProcessor subclasses use a soft bypass with fade-out to avoid clicks.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeBypassed | Number | yes | true to bypass, false to enable | -- |

**Cross References:**
- `$API.Effect.isBypassed$`

## exportState

**Signature:** `String exportState()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Base64 encoding involves heap allocation and string construction.
**Minimal Example:** `var state = {obj}.exportState();`

**Description:**
Serializes the entire processor state (all parameters, internal data, child processors) as a Base64 string. This captures the full module state, not just script controls.

**Parameters:**
None.

**Cross References:**
- `$API.Effect.restoreState$`
- `$API.Effect.exportScriptControls$`

## restoreState

**Signature:** `void restoreState(String base64State)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Suspends audio processing, kills voices, and waits for audio thread clearance before restoring. Heavy operation.
**Minimal Example:** `{obj}.restoreState(savedState);`

**Description:**
Restores the full processor state from a Base64 string previously obtained via `exportState()`. This is a heavy operation that acquires a suspension ticket, kills all active voices, and waits for the audio thread to clear before performing the restore. Reports a script error if the Base64 string cannot be parsed into a valid ValueTree.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| base64State | String | no | Base64-encoded processor state | Must be a valid state from exportState() |

**Cross References:**
- `$API.Effect.exportState$`
- `$API.Effect.restoreScriptControls$`

## exportScriptControls

**Signature:** `String exportScriptControls()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Base64 encoding involves heap allocation and string construction.
**Minimal Example:** `var controls = {obj}.exportScriptControls();`

**Description:**
Serializes the script UI control values of the wrapped effect as a Base64 string. Only works on Script FX modules -- calling this on a built-in (non-scripted) effect throws a script error with the message "exportScriptControls can only be used on Script Processors". Unlike `exportState()` which captures the entire processor state, this captures only script-defined control values.

**Parameters:**
None.

**Cross References:**
- `$API.Effect.restoreScriptControls$`
- `$API.Effect.exportState$`

## restoreScriptControls

**Signature:** `void restoreScriptControls(String base64Controls)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** State deserialization involves heap allocation and property restoration.
**Minimal Example:** `{obj}.restoreScriptControls(savedControls);`

**Description:**
Restores script UI control values from a Base64 string previously obtained via `exportScriptControls()`. Only works on Script FX modules -- calling this on a built-in (non-scripted) effect throws a script error with the message "restoreScriptControls can only be used on Script Processors".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| base64Controls | String | no | Base64-encoded script control state | Must be from exportScriptControls() |

**Cross References:**
- `$API.Effect.exportScriptControls$`
- `$API.Effect.restoreState$`

## getCurrentLevel

**Signature:** `float getCurrentLevel(bool leftChannel)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var level = {obj}.getCurrentLevel(true);`

**Description:**
Returns the current output level of the effect for the specified channel. Pass true for the left channel output, false for the right channel output. The values are read from the processor's DisplayValues struct, which is updated during audio processing.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| leftChannel | Integer | no | true for left output channel, false for right | -- |

**Cross References:**
- `$API.Effect.isSuspended$`

## addModulator

**Signature:** `var addModulator(var chainIndex, var typeName, var modName)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new processor in the module tree (heap allocation, module tree modification via ModuleHandler.addModule).
**Minimal Example:** `var mod = {obj}.addModulator(0, "LFO", "MyLFO");`

**Description:**
Adds a new modulator of the specified type to one of the effect's internal modulator chains. The `chainIndex` selects which child processor chain to add to (typically 0 for the first modulation chain). The `typeName` must be a valid modulator type name (e.g., `"LFO"`, `"Velocity"`, `"Constant"`). Returns a `Modulator` handle to the newly created modulator, or `undefined` if creation fails. Reports a script error if the chain index does not correspond to a valid modulator chain.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Index of the modulator chain to add to | Must reference a valid ModulatorChain child processor |
| typeName | String | no | The modulator type to create | Must be a valid modulator type name |
| modName | String | no | ID for the new modulator | -- |

**Pitfalls:**
- [BUG] Returns `undefined` without error if the module creation fails internally (e.g., invalid type name). Check the return value before using the handle.

**Cross References:**
- `$API.Effect.getModulatorChain$`
- `$API.Effect.addGlobalModulator$`
- `$API.Synth.addModulator$`

## getModulatorChain

**Signature:** `var getModulatorChain(var chainIndex)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new ScriptingModulator wrapper on the heap.
**Minimal Example:** `var chain = {obj}.getModulatorChain(0);`

**Description:**
Returns a `Modulator` handle to the modulator chain at the specified child processor index. This handle can be used to inspect or manipulate the chain's parameters. Reports a script error if no modulator chain exists at the given index.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Index of the modulator chain | Must reference a valid ModulatorChain child processor |

**Cross References:**
- `$API.Effect.addModulator$`

## addGlobalModulator

**Signature:** `var addGlobalModulator(var chainIndex, var globalMod, String modName)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a GlobalTimeVariantModulator receiver and connects it to the global source (heap allocation, module tree modification).
**Minimal Example:** `var mod = {obj}.addGlobalModulator(0, globalLfo, "GlobalLFO");`

**Description:**
Creates a `GlobalTimeVariantModulator` receiver in the specified modulator chain and connects it to an existing global modulator. The `globalMod` parameter must be a `Modulator` handle referencing a modulator inside a `GlobalModulatorContainer`. The receiver continuously tracks the global modulator's output value. Returns a `Modulator` handle to the newly created receiver, or `undefined` if the connection fails.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Index of the modulator chain to add to | Must reference a valid ModulatorChain child processor |
| globalMod | ScriptObject | no | Modulator handle referencing a global modulator | Must be inside a GlobalModulatorContainer |
| modName | String | no | ID for the new receiver modulator | -- |

**Pitfalls:**
- [BUG] Silently returns `undefined` without error if `globalMod` is not a valid `Modulator` handle. No script error is reported -- the call appears to succeed but produces no result.

**Cross References:**
- `$API.Effect.addStaticGlobalModulator$`
- `$API.Effect.addModulator$`
- `$API.Synth.addGlobalModulator$`

## addStaticGlobalModulator

**Signature:** `var addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a GlobalStaticTimeVariantModulator receiver and connects it (heap allocation, module tree modification).
**Minimal Example:** `var mod = {obj}.addStaticGlobalModulator(0, globalLfo, "StaticLFO");`

**Description:**
Creates a `GlobalStaticTimeVariantModulator` receiver in the specified modulator chain and connects it to an existing global modulator. Unlike `addGlobalModulator` which continuously tracks the source, the static variant uses a constant value snapshot that is only updated at voice start. The `timeVariantMod` parameter must be a `Modulator` handle referencing a modulator inside a `GlobalModulatorContainer`. Returns a `Modulator` handle to the newly created receiver, or `undefined` if the connection fails.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Index of the modulator chain to add to | Must reference a valid ModulatorChain child processor |
| timeVariantMod | ScriptObject | no | Modulator handle referencing a global modulator | Must be inside a GlobalModulatorContainer |
| modName | String | no | ID for the new receiver modulator | -- |

**Pitfalls:**
- [BUG] Silently returns `undefined` without error if `timeVariantMod` is not a valid `Modulator` handle. No script error is reported.

**Cross References:**
- `$API.Effect.addGlobalModulator$`
- `$API.Synth.addStaticGlobalModulator$`

## getDraggableFilterData

**Signature:** `var getDraggableFilterData()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Creates a DynamicObject on the heap for the JSON return value.
**Minimal Example:** `var data = {obj}.getDraggableFilterData();`

**Description:**
Returns the current draggable filter configuration as a JSON object for effects that implement the `ProcessorWithCustomFilterStatistics` interface (Script FX, Hardcoded FX, Polyphonic Filter). The returned object contains properties describing filter band count, parameter mapping, drag interaction bindings, and FFT display settings. Returns `undefined` for effects that do not support this interface.

**Parameters:**
None.

**Pitfalls:**
- Silently returns `undefined` for effects that do not implement filter statistics (most built-in effects). No error is reported, so the caller should check the return value.

**Cross References:**
- `$API.Effect.setDraggableFilterData$`

## setDraggableFilterData

**Signature:** `void setDraggableFilterData(var filterData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Modifies internal processor filter statistics state.
**Minimal Example:** `{obj}.setDraggableFilterData(filterConfig);`

**Description:**
Configures the draggable filter visualization for effects that implement the `ProcessorWithCustomFilterStatistics` interface (Script FX, Hardcoded FX, Polyphonic Filter). The `filterData` JSON object describes filter band layout, parameter mapping, mouse drag actions, and FFT display settings. Silently does nothing on effects that do not support this interface.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| filterData | JSON | no | Filter configuration object | See Callback Properties for schema |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| NumFilterBands | Integer | Number of filter bands to display |
| FilterDataSlot | Integer | ExternalData FilterCoefficients slot index for visualization |
| FirstBandOffset | Integer | Attribute index where band parameters start |
| TypeList | Array | Array of filter type display names (e.g., ["Low Pass", "High Pass", "Peak"]) |
| ParameterOrder | Array | Parameter names per band in attribute order (e.g., ["Gain", "Freq", "Q", "Enabled", "Type"]) |
| FFTDisplayBufferIndex | Integer | Display buffer index for FFT overlay (-1 to disable) |
| DragActions | JSON | Mouse interaction to parameter mapping |
| DragActions.DragX | String | Parameter controlled by horizontal drag (e.g., "Freq") |
| DragActions.DragY | String | Parameter controlled by vertical drag (e.g., "Gain") |
| DragActions.ShiftDrag | String | Parameter controlled by shift+drag (e.g., "Q") |
| DragActions.DoubleClick | String | Parameter toggled by double-click (e.g., "Enabled") |
| DragActions.RightClick | String | Parameter controlled by right-click (empty string for none) |

**Pitfalls:**
- Silently does nothing on effects that do not implement filter statistics (most built-in effects). No error is reported.

**Cross References:**
- `$API.Effect.getDraggableFilterData$`

**Example:**
```javascript:draggable-filter-config
// Title: Configure draggable filter visualization for a Script FX
const var fx = Synth.getEffect("ScriptFX");

fx.setDraggableFilterData({
    "NumFilterBands": 3,
    "FilterDataSlot": 0,
    "FirstBandOffset": 0,
    "TypeList": ["Low Pass", "High Pass", "Peak"],
    "ParameterOrder": ["Gain", "Freq", "Q", "Enabled", "Type"],
    "FFTDisplayBufferIndex": -1,
    "DragActions": {
        "DragX": "Freq",
        "DragY": "Gain",
        "ShiftDrag": "Q",
        "DoubleClick": "Enabled",
        "RightClick": ""
    }
});
```
```json:testMetadata:draggable-filter-config
{
  "testable": false,
  "skipReason": "Requires a Script FX or Polyphonic Filter module in the module tree that implements ProcessorWithCustomFilterStatistics"
}
```
