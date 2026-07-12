# Modulator -- Method Entries

## exists

**Signature:** `Integer exists()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var valid = {obj}.exists();`

**Description:**
Returns whether the modulator reference is valid. Returns 1 if the internal C++ modulator pointer is non-null and the modulator has not been deleted from the module tree, 0 otherwise. Use this to verify references before calling other methods, particularly when the modulator may have been removed dynamically.

**Parameters:**
None.

**Cross References:**
- `$API.Modulator.getId$`

---

## getId

**Signature:** `String getId()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return involves atomic ref-count operations.
**Minimal Example:** `var id = {obj}.getId();`

**Description:**
Returns the ID (name) of the modulator as assigned in the HISE module tree. Returns an empty string if the modulator reference is invalid.

**Parameters:**
None.

**Cross References:**
- `$API.Modulator.getType$`
- `$API.Modulator.exists$`

---

## getType

**Signature:** `String getType()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return involves atomic ref-count operations.
**Minimal Example:** `var type = {obj}.getType();`

**Description:**
Returns the C++ type name of the modulator (e.g., "LFO", "AHDSR", "Velocity", "Constant"). This is the module type identifier, not the user-assigned name. Returns an empty string if the modulator reference is invalid.

**Parameters:**
None.

**Cross References:**
- `$API.Modulator.getId$`

---

## setAttribute

**Signature:** `undefined setAttribute(Number index, Number value)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Uses thread-appropriate notification type via ProcessorHelpers::getAttributeNotificationType(), making this safe to call from any callback including the audio thread.
**Minimal Example:** `{obj}.setAttribute({obj}.Frequency, 2.5);`

**Description:**
Sets a modulator attribute by its parameter index. Use the dynamic parameter constants (e.g., `mod.Frequency`, `mod.FadeIn`) or the result of `getAttributeIndex()` as the index. The bracket-write operator `mod["Frequency"] = 2.5` is equivalent to calling `setAttribute`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Number | yes | Parameter index | 0 to getNumAttributes()-1 |
| value | Number | yes | New attribute value | Range depends on parameter |

**Cross References:**
- `$API.Modulator.getAttribute$`
- `$API.Modulator.getAttributeIndex$`
- `$API.Modulator.getNumAttributes$`

---

## getAttribute

**Signature:** `Double getAttribute(Number parameterIndex)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var freq = {obj}.getAttribute({obj}.Frequency);`

**Description:**
Returns the current value of a modulator attribute at the given parameter index.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Number | yes | Parameter index | 0 to getNumAttributes()-1 |

**Cross References:**
- `$API.Modulator.setAttribute$`
- `$API.Modulator.getAttributeId$`

---

## getAttributeId

**Signature:** `String getAttributeId(Number parameterIndex)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return involves atomic ref-count operations.
**Minimal Example:** `var name = {obj}.getAttributeId(0);`

**Description:**
Returns the name of the attribute at the given parameter index. Useful for iterating all parameters by index to build dynamic UIs or preset systems.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Number | yes | Parameter index | 0 to getNumAttributes()-1 |

**Cross References:**
- `$API.Modulator.getAttributeIndex$`
- `$API.Modulator.getNumAttributes$`

---

## getAttributeIndex

**Signature:** `Integer getAttributeIndex(String parameterId)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String parameter involves atomic ref-count operations.
**Minimal Example:** `var idx = {obj}.getAttributeIndex("Frequency");`

**Description:**
Returns the parameter index for a given attribute name. Returns -1 if the name is not found. This is the reverse lookup of `getAttributeId()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterId | String | yes | Parameter name | Must match an existing attribute name |

**Cross References:**
- `$API.Modulator.getAttributeId$`
- `$API.Modulator.setAttribute$`

---

## getNumAttributes

**Signature:** `Integer getNumAttributes()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var count = {obj}.getNumAttributes();`

**Description:**
Returns the total number of parameters exposed by this modulator. The available parameters depend on the modulator type (e.g., an LFO has Frequency, FadeIn, TempoSync, etc.).

**Parameters:**
None.

**Cross References:**
- `$API.Modulator.getAttribute$`
- `$API.Modulator.getAttributeId$`

---

## setBypassed

**Signature:** `undefined setBypassed(Number shouldBeBypassed)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls setBypassed with sendNotification (synchronous listener dispatch) and sendOtherChangeMessage for UI update.
**Minimal Example:** `{obj}.setBypassed(1);`

**Description:**
Enables or disables the bypass state of the modulator. When bypassed, the modulator's output is not applied to its target. Also dispatches a bypass change notification to update the UI.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeBypassed | Number | yes | 1 to bypass, 0 to activate | -- |

**Cross References:**
- `$API.Modulator.isBypassed$`

---

## isBypassed

**Signature:** `Integer isBypassed()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var bp = {obj}.isBypassed();`

**Description:**
Returns the current bypass state. 1 if bypassed, 0 if active.

**Parameters:**
None.

**Cross References:**
- `$API.Modulator.setBypassed$`

---

## setIntensity

**Signature:** `undefined setIntensity(Number newIntensity)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Writes to a lock-free LinearSmoothedValue and dispatches notification asynchronously.
**Minimal Example:** `{obj}.setIntensity(0.5);`

**Description:**
Sets the modulation intensity. The valid range depends on the modulation mode of the parent chain:

- **GainMode:** 0.0 to 1.0 (clamped)
- **PitchMode:** -12.0 to 12.0 in semitones (clamped, internally stored as -1.0 to 1.0)
- **PanMode / GlobalMode / OffsetMode / CombinedMode:** -1.0 to 1.0 (clamped)

Values outside the valid range are clamped automatically without error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newIntensity | Number | yes | New intensity value | Range depends on modulation mode |

**Cross References:**
- `$API.Modulator.getIntensity$`

---

## getIntensity

**Signature:** `Double getIntensity()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var intensity = {obj}.getIntensity();`

**Description:**
Returns the current modulation intensity. For PitchMode modulators, the internal normalized value (-1.0..1.0) is converted back to semitones (-12.0..12.0). For all other modes, returns the raw intensity value.

**Parameters:**
None.

**Cross References:**
- `$API.Modulator.setIntensity$`

---

## setIsBipolar

**Signature:** `undefined setIsBipolar(Number shouldBeBipolar)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setIsBipolar(1);`

**Description:**
Sets whether the modulator operates in bipolar mode. In GainMode, unipolar output is 0..1 while bipolar output is -1..1. Affects how the modulation intensity value is applied to the destination signal.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeBipolar | Number | yes | 1 for bipolar, 0 for unipolar | -- |

**Cross References:**
- `$API.Modulator.isBipolar$`

---

## isBipolar

**Signature:** `Integer isBipolar()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var bp = {obj}.isBipolar();`

**Description:**
Returns whether the modulator is in bipolar mode. 1 if bipolar, 0 if unipolar.

**Parameters:**
None.

**Cross References:**
- `$API.Modulator.setIsBipolar$`

---

## getCurrentLevel

**Signature:** `Double getCurrentLevel()`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Reads display values written by the audio thread. Read-only with no synchronization needed.
**Minimal Example:** `var level = {obj}.getCurrentLevel();`

**Description:**
Returns the current display output value of the modulator. For PitchMode modulators, the raw pitch factor (0.5..2.0 internal range) is converted to a 0.0..1.0 display range. For all other modes, returns the raw output value. This is intended for UI display and may lag behind the actual audio-thread value by one buffer.

**Parameters:**
None.

**Cross References:**
- `$API.Modulator.getIntensity$`

---

## exportState

**Signature:** `String exportState()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Serializes the full processor state to XML and encodes as base64, involving heap allocations.
**Minimal Example:** `var state = {obj}.exportState();`

**Description:**
Serializes the complete modulator state (all attributes, bypass state, child processors) to a base64-encoded string. Use with `restoreState()` to save and restore modulator configurations at runtime.

**Parameters:**
None.

**Cross References:**
- `$API.Modulator.restoreState$`

---

## restoreState

**Signature:** `undefined restoreState(String base64State)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Parses XML from base64 and applies full processor state with property changes.
**Minimal Example:** `{obj}.restoreState(savedState);`

**Description:**
Restores the complete modulator state from a base64-encoded string previously obtained from `exportState()`. Reports a script error if the base64 string is invalid or cannot be decoded to a valid ValueTree.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| base64State | String | no | Base64-encoded state from exportState() | Must be a valid base64 processor state |

**Cross References:**
- `$API.Modulator.exportState$`

---

## exportScriptControls

**Signature:** `String exportScriptControls()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Serializes UI control values to base64, involving heap allocations.
**Minimal Example:** `var controls = {obj}.exportScriptControls();`

**Description:**
Serializes the UI control values of a script modulator to a base64-encoded string. Only works on modulators that are script processors (Script Voice Start Modulator, Script Time Variant Modulator, Script Envelope Modulator). Reports a script error if called on a non-script modulator.

**Parameters:**
None.

**Pitfalls:**
- Only functional on script modulators (ProcessorWithScriptingContent). Calling on a built-in modulator type (LFO, AHDSR, etc.) reports a script error.

**Cross References:**
- `$API.Modulator.restoreScriptControls$`

---

## restoreScriptControls

**Signature:** `undefined restoreScriptControls(String base64Controls)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Parses base64 and restores UI control values with property changes.
**Minimal Example:** `{obj}.restoreScriptControls(savedControls);`

**Description:**
Restores the UI control values of a script modulator from a base64-encoded string previously obtained from `exportScriptControls()`. Restores only the control values without recompiling the script. Reports a script error if called on a non-script modulator.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| base64Controls | String | no | Base64-encoded control state from exportScriptControls() | Must be a valid base64 control state |

**Pitfalls:**
- Only functional on script modulators (ProcessorWithScriptingContent). Calling on a built-in modulator type reports a script error.

**Cross References:**
- `$API.Modulator.exportScriptControls$`

---

## connectToGlobalModulator

**Signature:** `Integer connectToGlobalModulator(String globalModulationContainerId, String modulatorId)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Involves string concatenation and global modulator connection logic with potential message dispatching.
**Minimal Example:** `var ok = {obj}.connectToGlobalModulator("GlobalModContainer", "LFO1");`

**Description:**
Connects a global receiver modulator to a source modulator inside a GlobalModulatorContainer. Only works when the modulator is a global receiver type (GlobalVoiceStartModulator, GlobalTimeVariantModulator, GlobalStaticTimeVariantModulator, GlobalEnvelopeModulator). The two string parameters are concatenated with a colon separator internally (`containerId:modulatorId`). Reports a script error if the modulator is not a global receiver type.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| globalModulationContainerId | String | no | ID of the GlobalModulatorContainer | Must match an existing container |
| modulatorId | String | no | ID of the source modulator inside the container | Must match an existing modulator in the container |

**Pitfalls:**
- Only works on global receiver modulator types. Calling on a regular modulator (LFO, AHDSR, etc.) reports a script error.

**Cross References:**
- `$API.Modulator.getGlobalModulatorId$`
- `$API.Modulator.addGlobalModulator$`

---

## getGlobalModulatorId

**Signature:** `String getGlobalModulatorId()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a new String via concatenation ("ContainerName:ModulatorName").
**Minimal Example:** `var globalId = {obj}.getGlobalModulatorId();`

**Description:**
Returns the identifier of the connected global modulator in the format `"ContainerName:ModulatorName"`. Only works on modulator types whose type name starts with "Global". Returns an empty string for non-global modulators without reporting an error.

**Parameters:**
None.

**Pitfalls:**
- [BUG] Returns an empty string silently when called on a non-global modulator type, with no error or indication that the call was inappropriate.

**Cross References:**
- `$API.Modulator.connectToGlobalModulator$`

---

## setMatrixProperties

**Signature:** `undefined setMatrixProperties(JSON matrixData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Involves JSON parsing, property storage, and synchronous notification dispatch.
**Minimal Example:** `{obj}.setMatrixProperties({"MinValue": 0.0, "MaxValue": 1.0});`

**Description:**
Sets the matrix modulation range properties for a MatrixModulator. Converts the JSON data to `RangeData` and stores it on the `GlobalModulatorContainer`'s matrix properties system. Only functional when the modulator is a MatrixModulator instance; silently does nothing for other modulator types.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| matrixData | JSON | no | Range data for the matrix modulator | Must be a valid RangeData JSON structure |

**Pitfalls:**
- [BUG] Silently does nothing when called on a non-MatrixModulator. No error is reported, so the call appears to succeed.

**Cross References:**
- `$API.Modulator.connectToGlobalModulator$`

---

## asTableProcessor

**Signature:** `ScriptObject asTableProcessor()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new ScriptingTableProcessor wrapper on the heap.
**Minimal Example:** `var tp = {obj}.asTableProcessor();`

**Description:**
Converts this modulator to a `TableProcessor` handle if the underlying modulator implements `LookupTableProcessor` (e.g., TableEnvelope, table-based modulators). Returns `undefined` if the modulator is not a table processor type. Does not report an error on failure -- check the return value with `isDefined()`.

**Parameters:**
None.

**Example:**
```javascript:as-table-processor
// Title: Access a modulator's lookup table for editing
const var mod = Synth.getModulator("TableEnvelope1");
const var tp = mod.asTableProcessor();

if (isDefined(tp))
{
    tp.addTablePoint(0, 0.5, 0.8);
    tp.reset(0);
}
```
```json:testMetadata:as-table-processor
{
  "testable": false,
  "skipReason": "Requires a table-based modulator in the module tree"
}
```

**Cross References:**
None.

---

## addModulator

**Signature:** `ScriptObject addModulator(Integer chainIndex, String typeName, String modName)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new module via ModuleHandler, coordinates with KillStateHandler, heap allocates the wrapper.
**Minimal Example:** `var lfo = {obj}.addModulator(0, "LFO", "MyLFO");`

**Description:**
Adds a new child modulator to one of this modulator's internal chains. The `chainIndex` refers to the modulator's child processor slots (the available indices depend on the specific modulator type). Returns a `Modulator` handle for the newly created modulator, or `undefined` on failure. Reports a script error if the chain index is invalid.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Index of the child modulator chain | Must be a valid child chain index |
| typeName | String | no | Type of modulator to create | Must be a valid modulator type name (e.g., "LFO", "Velocity", "Constant") |
| modName | String | no | ID for the new modulator | Should be unique within the module tree |

**Example:**
```javascript:add-modulator-to-chain
// Title: Add an LFO to a modulator's intensity chain
const var env = Synth.getModulator("GainModulation1");
const var lfo = env.addModulator(0, "LFO", "DynamicLFO");
lfo.setAttribute(lfo.Frequency, 3.0);
lfo.setIntensity(0.5);
```
```json:testMetadata:add-modulator-to-chain
{
  "testable": false,
  "skipReason": "Requires an existing modulator with child chains in the module tree"
}
```

**Cross References:**
- `$API.Modulator.addGlobalModulator$`
- `$API.Modulator.addStaticGlobalModulator$`
- `$API.Modulator.getModulatorChain$`

---

## addGlobalModulator

**Signature:** `ScriptObject addGlobalModulator(Integer chainIndex, ScriptObject globalMod, String modName)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a global receiver module via ModuleHandler, coordinates with KillStateHandler, heap allocates the wrapper.
**Minimal Example:** `var receiver = {obj}.addGlobalModulator(0, globalMod, "GlobalLFOReceiver");`

**Description:**
Creates a time-variant global modulator receiver in one of this modulator's internal chains and connects it to the specified source modulator. The `globalMod` parameter must be a `Modulator` handle referencing a modulator inside a `GlobalModulatorContainer`. Returns a `Modulator` handle for the newly created receiver, or `undefined` on failure.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Index of the child modulator chain | Must be a valid child chain index |
| globalMod | ScriptObject | no | Modulator handle referencing the source modulator in a GlobalModulatorContainer | Must be a valid Modulator inside a GlobalModulatorContainer |
| modName | String | no | ID for the new receiver modulator | Should be unique within the module tree |

**Cross References:**
- `$API.Modulator.addStaticGlobalModulator$`
- `$API.Modulator.connectToGlobalModulator$`
- `$API.Modulator.addModulator$`

---

## addStaticGlobalModulator

**Signature:** `ScriptObject addStaticGlobalModulator(Integer chainIndex, ScriptObject timeVariantMod, String modName)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a static global receiver module via ModuleHandler, coordinates with KillStateHandler, heap allocates the wrapper.
**Minimal Example:** `var receiver = {obj}.addStaticGlobalModulator(0, globalMod, "StaticLFOReceiver");`

**Description:**
Creates a static time-variant global modulator receiver in one of this modulator's internal chains. Unlike `addGlobalModulator`, the static variant samples the source modulator's value only at voice start, rather than continuously tracking it. This is more CPU-efficient when per-block modulation updates are not needed. The `timeVariantMod` parameter must reference a modulator inside a `GlobalModulatorContainer`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Index of the child modulator chain | Must be a valid child chain index |
| timeVariantMod | ScriptObject | no | Modulator handle referencing the source modulator in a GlobalModulatorContainer | Must be a valid Modulator inside a GlobalModulatorContainer |
| modName | String | no | ID for the new static receiver modulator | Should be unique within the module tree |

**Cross References:**
- `$API.Modulator.addGlobalModulator$`
- `$API.Modulator.addModulator$`
- `$API.Modulator.connectToGlobalModulator$`

---

## getModulatorChain

**Signature:** `ScriptObject getModulatorChain(Integer chainIndex)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new ScriptingModulator wrapper on the heap.
**Minimal Example:** `var chain = {obj}.getModulatorChain(0);`

**Description:**
Returns a `Modulator` handle for the internal modulator chain at the given index. ModulatorChain inherits from Modulator (via EnvelopeModulator), so the returned handle can itself have attributes set, be bypassed, and have child modulators added. The available chain indices depend on the specific modulator type. Reports a script error if the chain index is invalid.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Index of the child modulator chain | Must be a valid child chain index |

**Cross References:**
- `$API.Modulator.addModulator$`
- `$API.Modulator.addGlobalModulator$`
- `$API.Modulator.addStaticGlobalModulator$`
