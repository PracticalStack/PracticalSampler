# MidiProcessor -- Method Entries

## asMidiPlayer

**Signature:** `var asMidiPlayer()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new ScriptedMidiPlayer wrapper object (heap allocation).
**Minimal Example:** `var player = {obj}.asMidiPlayer();`

**Description:**
Casts this MidiProcessor handle to a MidiPlayer handle. Performs a runtime type check -- succeeds only if the underlying module is a MidiPlayer, otherwise throws a script error. The returned MidiPlayer object provides MIDI file playback, editing, and visualization methods. This is the inverse of `MidiPlayer.asMidiProcessor()`.

**Parameters:**

(none)

**Pitfalls:**
- Throws a script error ("The module is not a MIDI player") if the underlying module is not a MidiPlayer. Only use on modules known to be MidiPlayer instances.

**Cross References:**
- `$API.MidiPlayer.asMidiProcessor$`

---

## exists

**Signature:** `bool exists()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var valid = {obj}.exists();`

**Description:**
Returns whether the underlying MIDI processor module reference is still valid. Returns false and prints an error message to the console if the module has been deleted or the reference was created with an invalid module ID.

**Parameters:**

(none)

**Cross References:**
None.

---

## exportScriptControls

**Signature:** `String exportScriptControls()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** ValueTree serialization and base64 encoding involve heap allocations.
**Minimal Example:** `var controls = {obj}.exportScriptControls();`

**Description:**
Exports only the UI control values (scripting content) of the MIDI processor as a base64-encoded string. Only works when the underlying module is a script processor (JavascriptMidiProcessor). Unlike `exportState()`, this exports only knob/slider/button values without the full processor state, and restoring via `restoreScriptControls()` does not trigger script recompilation.

**Parameters:**

(none)

**Pitfalls:**
- Only works on script processors. Calling on built-in MIDI modules (Transposer, Arpeggiator, etc.) throws: "exportScriptControls can only be used on Script Processors". Use `exportState()` for non-script modules.

**Cross References:**
- `$API.MidiProcessor.restoreScriptControls$`
- `$API.MidiProcessor.exportState$`

---

## exportState

**Signature:** `String exportState()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** ValueTree serialization and base64 encoding involve heap allocations.
**Minimal Example:** `var state = {obj}.exportState();`

**Description:**
Exports the full processor state (all parameters and internal state) as a base64-encoded string. The string can be passed to `restoreState()` to restore the saved state. Works on any MIDI processor type, not just script processors.

**Parameters:**

(none)

**Cross References:**
- `$API.MidiProcessor.restoreState$`
- `$API.MidiProcessor.exportScriptControls$`

---

## getAttribute

**Signature:** `float getAttribute(int parameterIndex)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var value = {obj}.getAttribute({obj}.Intensity);`

**Description:**
Returns the current value of the parameter at the given index. Use the dynamic constants (e.g., `mp.Intensity`) for named access instead of raw index numbers. Returns 0.0 if the object reference is invalid.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Integer | no | Zero-based parameter index. Use named constants for readability. | 0 to getNumAttributes()-1 |

**Cross References:**
- `$API.MidiProcessor.setAttribute$`
- `$API.MidiProcessor.getAttributeId$`
- `$API.MidiProcessor.getAttributeIndex$`

---

## getAttributeId

**Signature:** `String getAttributeId(int parameterIndex)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return involves atomic ref-count operations.
**Minimal Example:** `var name = {obj}.getAttributeId(0);`

**Description:**
Returns the string identifier of the parameter at the given index. Useful for debugging or building dynamic parameter UIs that enumerate all parameters by name.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Integer | no | Zero-based parameter index. | 0 to getNumAttributes()-1 |

**Cross References:**
- `$API.MidiProcessor.getAttributeIndex$`
- `$API.MidiProcessor.getAttribute$`

---

## getAttributeIndex

**Signature:** `int getAttributeIndex(String parameterName)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String parameter involves atomic ref-count operations.
**Minimal Example:** `var idx = {obj}.getAttributeIndex("Intensity");`

**Description:**
Returns the integer index of the parameter with the given string identifier. The inverse of `getAttributeId()`. Returns -1 if no parameter matches the given name.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterName | String | no | String identifier of the parameter. | Must match an existing parameter name |

**Cross References:**
- `$API.MidiProcessor.getAttributeId$`
- `$API.MidiProcessor.getAttribute$`

---

## getId

**Signature:** `String getId()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return involves atomic ref-count operations.
**Minimal Example:** `var name = {obj}.getId();`

**Description:**
Returns the module ID of the underlying MIDI processor as a string. This is the ID assigned in the HISE module tree (e.g., "Arpeggiator1").

**Parameters:**

(none)

**Cross References:**
None.

---

## getNumAttributes

**Signature:** `int getNumAttributes()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var count = {obj}.getNumAttributes();`

**Description:**
Returns the total number of parameters on the underlying MIDI processor module. The count depends on the specific module type.

**Parameters:**

(none)

**Cross References:**
- `$API.MidiProcessor.getAttribute$`
- `$API.MidiProcessor.getAttributeId$`

---

## isBypassed

**Signature:** `bool isBypassed()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var bypassed = {obj}.isBypassed();`

**Description:**
Returns whether the MIDI processor is currently bypassed.

**Parameters:**

(none)

**Cross References:**
- `$API.MidiProcessor.setBypassed$`

---

## restoreScriptControls

**Signature:** `void restoreScriptControls(String base64Controls)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Base64 decoding, ValueTree parsing, and state restoration involve heap allocations.
**Minimal Example:** `{obj}.restoreScriptControls(savedControls);`

**Description:**
Restores UI control values from a base64-encoded string previously generated by `exportScriptControls()`. Only works when the underlying module is a script processor (JavascriptMidiProcessor). Restores only scripting content (knob/slider/button values) without triggering script recompilation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| base64Controls | String | no | Base64-encoded string from exportScriptControls(). | Must be a valid base64 processor state |

**Pitfalls:**
- Only works on script processors. Calling on built-in MIDI modules throws: "restoreScriptControls can only be used on Script Processors".

**Cross References:**
- `$API.MidiProcessor.exportScriptControls$`
- `$API.MidiProcessor.restoreState$`

---

## restoreState

**Signature:** `void restoreState(String base64State)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Base64 decoding, ValueTree parsing, and state restoration involve heap allocations.
**Minimal Example:** `{obj}.restoreState(savedState);`

**Description:**
Restores the full processor state from a base64-encoded string previously generated by `exportState()`. The base64 string is validated by parsing it into a ValueTree before restoration. Works on any MIDI processor type.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| base64State | String | no | Base64-encoded string from exportState(). | Must be a valid base64 processor state |

**Cross References:**
- `$API.MidiProcessor.exportState$`
- `$API.MidiProcessor.restoreScriptControls$`

---

## setAttribute

**Signature:** `void setAttribute(int parameterIndex, float value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Triggers ValueTree property change with notifications, which may involve string lookups and listener dispatch.
**Minimal Example:** `{obj}.setAttribute({obj}.Intensity, 0.5);`

**Description:**
Sets the value of the parameter at the given index. Use the dynamic constants (e.g., `mp.Intensity`) for named access instead of raw index numbers. The notification type is determined automatically based on the calling context. Bracket assignment syntax (`mp["Intensity"] = 0.5`) also delegates to this method.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Integer | no | Zero-based parameter index. Use named constants for readability. | 0 to getNumAttributes()-1 |
| value | Number | no | New parameter value. | Module-dependent range |

**Cross References:**
- `$API.MidiProcessor.getAttribute$`
- `$API.MidiProcessor.getAttributeIndex$`

---

## setBypassed

**Signature:** `void setBypassed(bool shouldBeBypassed)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Sends bypass notification and dispatches a ProcessorChangeEvent, which involves listener callbacks.
**Minimal Example:** `{obj}.setBypassed(true);`

**Description:**
Sets the bypass state of the MIDI processor. When bypassed, the module does not process MIDI events. Sends both a bypass notification and a ProcessorChangeEvent dispatch.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeBypassed | Integer | no | True to bypass, false to enable. | Boolean value |

**Cross References:**
- `$API.MidiProcessor.isBypassed$`
