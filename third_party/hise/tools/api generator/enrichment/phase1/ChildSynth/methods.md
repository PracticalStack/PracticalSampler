# ChildSynth -- Method Documentation

## addGlobalModulator

**Signature:** `var addGlobalModulator(var chainIndex, var globalMod, String modName)`
**Return Type:** `ScriptingModulator`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to ModuleHandler.addAndConnectToGlobalModulator which modifies the module tree, creates objects, and acquires locks.
**Minimal Example:** `var mod = {obj}.addGlobalModulator(1, globalLfo, "GainLFO");`

**Description:**
Adds a global modulator receiver to the specified modulator chain of this synth. The receiver tracks the specified global modulator source, providing per-voice modulation values. The `globalMod` parameter must be a reference to a modulator inside a GlobalModulatorContainer, obtained via `Synth.getModulator()`. Returns the newly created modulator as a Modulator reference, or undefined if creation fails.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Modulator chain index (1 = Gain, 2 = Pitch) | Must be a valid ModulatorChain index |
| globalMod | ScriptObject | no | Reference to a modulator inside a GlobalModulatorContainer | Must be a ScriptingModulator |
| modName | String | no | Name for the new modulator receiver | -- |

**Pitfalls:**
- [BUG] If `globalMod` is not a ScriptingModulator (fails the dynamic_cast), the method silently returns undefined without an error message.

**Cross References:**
- `$API.ChildSynth.addStaticGlobalModulator$`
- `$API.ChildSynth.addModulator$`
- `$API.ChildSynth.getModulatorChain$`

---

## addModulator

**Signature:** `var addModulator(var chainIndex, var typeName, var modName)`
**Return Type:** `ScriptingModulator`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to ModuleHandler.addModule which modifies the module tree and acquires locks.
**Minimal Example:** `var mod = {obj}.addModulator(1, "LFOModulator", "GainLFO");`

**Description:**
Creates and adds a new modulator of the specified type to the given modulator chain. The `typeName` must match the exact C++ class name of the modulator type (e.g., "LFOModulator", "Velocity", "ConstantModulator"). Returns the newly created modulator as a Modulator reference, or undefined if creation fails.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Modulator chain index (1 = Gain, 2 = Pitch) | Must be a valid ModulatorChain index |
| typeName | String | no | C++ class name of the modulator type to create | Must match an existing modulator type |
| modName | String | no | Name for the new modulator | -- |

**Pitfalls:**
- [BUG] The `typeName` must be the exact C++ class name, not a human-readable name. Using incorrect type names causes silent failure (returns undefined, no error message).

**Cross References:**
- `$API.ChildSynth.addGlobalModulator$`
- `$API.ChildSynth.addStaticGlobalModulator$`
- `$API.ChildSynth.getModulatorChain$`

---

## addStaticGlobalModulator

**Signature:** `var addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName)`
**Return Type:** `ScriptingModulator`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to ModuleHandler.addAndConnectToGlobalModulator which modifies the module tree, creates objects, and acquires locks.
**Minimal Example:** `var mod = {obj}.addStaticGlobalModulator(1, globalLfo, "StaticGainLFO");`

**Description:**
Adds a static global modulator receiver to the specified modulator chain. Unlike `addGlobalModulator` which provides per-voice modulation, a static global modulator provides a single value per audio block (time-variant but not voice-variant). This is more efficient when per-voice resolution is not needed. The `timeVariantMod` must be a reference to a modulator inside a GlobalModulatorContainer. Returns the newly created modulator as a Modulator reference, or undefined if creation fails.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Modulator chain index (1 = Gain, 2 = Pitch) | Must be a valid ModulatorChain index |
| timeVariantMod | ScriptObject | no | Reference to a time-variant modulator in a GlobalModulatorContainer | Must be a ScriptingModulator |
| modName | String | no | Name for the new static modulator receiver | -- |

**Pitfalls:**
- [BUG] If `timeVariantMod` is not a ScriptingModulator (fails the dynamic_cast), the method silently returns undefined without an error message.

**Cross References:**
- `$API.ChildSynth.addGlobalModulator$`
- `$API.ChildSynth.addModulator$`
- `$API.ChildSynth.getModulatorChain$`

---

## asSampler

**Signature:** `var asSampler()`
**Return Type:** `Sampler`
**Call Scope:** safe
**Minimal Example:** `var sampler = {obj}.asSampler();`

**Description:**
Attempts to cast this ChildSynth to a Sampler reference. If the wrapped synth is a ModulatorSampler, returns a Sampler handle. If the synth is not a sampler type, returns undefined silently -- this is intentional so scripts can check the result at the scripting level without error spam.

**Parameters:**
None.

**Pitfalls:**
- Returns undefined (not an error) when the synth is not a ModulatorSampler. Always check the return value with `isDefined()` before using.
- [BUG] If the underlying object reference is invalid (synth was deleted), still creates a Sampler wrapping nullptr rather than returning undefined.

**Cross References:**
- `$API.Synth.getSampler$`
- `$API.ChildSynth.getChildSynthByIndex$`

---

## exists

**Signature:** `bool exists()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var valid = {obj}.exists();`

**Description:**
Checks whether the wrapped synth module still exists and has not been deleted. Returns true if the internal weak reference to the processor is non-null and valid, false otherwise. This calls `checkValidObject()` internally, which checks both `objectExists()` (non-null) and `objectDeleted()` (weak reference validity). Unlike most other methods, this does not throw a script error on invalid objects -- it is specifically designed to test validity safely.

**Parameters:**
None.

**Cross References:**
- `$API.Effect.exists$`
- `$API.MidiProcessor.exists$`

---

## exportState

**Signature:** `String exportState()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Base64 encoding and ValueTree serialization involve heap allocations and string construction.
**Minimal Example:** `var state = {obj}.exportState();`

**Description:**
Exports the complete processor state as a base64-encoded string. The state captures all parameters, modulator chain configuration, and internal processor state via `ProcessorHelpers::getBase64String`. The returned string can later be passed to `restoreState()` to reload the saved configuration.

**Parameters:**
None.

**Cross References:**
- `$API.ChildSynth.restoreState$`

---

## getAttribute

**Signature:** `float getAttribute(int parameterIndex)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var gain = {obj}.getAttribute({obj}.Gain);`

**Description:**
Returns the current value of the attribute at the specified parameter index. Standard ModulatorSynth indices: 0 = Gain (0.0-1.0), 1 = Balance (-100 to 100), 2 = VoiceLimit, 3 = KillFadeTime. Subclasses extend with additional parameters. Use the dynamic constants registered on the ChildSynth instance (e.g., `cs.Gain`) rather than raw index numbers.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Integer | no | Index of the attribute to get | Must be a valid parameter index for the wrapped processor |

**Cross References:**
- `$API.ChildSynth.setAttribute$`
- `$API.ChildSynth.getAttributeId$`
- `$API.ChildSynth.getAttributeIndex$`
- `$API.ChildSynth.getNumAttributes$`

---

## getAttributeId

**Signature:** `String getAttributeId(int parameterIndex)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var name = {obj}.getAttributeId(0);`

**Description:**
Returns the string identifier of the attribute at the specified parameter index. For example, index 0 returns "Gain", index 1 returns "Balance", etc. Useful for debugging or building dynamic parameter UIs.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Integer | no | Index of the attribute | Must be a valid parameter index |

**Cross References:**
- `$API.ChildSynth.getAttribute$`
- `$API.ChildSynth.setAttribute$`
- `$API.ChildSynth.getAttributeIndex$`
- `$API.ChildSynth.getNumAttributes$`

---

## getAttributeIndex

**Signature:** `int getAttributeIndex(String parameterId)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations on parameter lookup.
**Minimal Example:** `var idx = {obj}.getAttributeIndex("Gain");`

**Description:**
Returns the integer parameter index for the given string identifier. This is the reverse of `getAttributeId()`. Returns -1 if the parameter identifier is not found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterId | String | no | String identifier of the attribute (e.g., "Gain") | -- |

**Cross References:**
- `$API.ChildSynth.getAttributeId$`
- `$API.ChildSynth.getAttribute$`
- `$API.ChildSynth.setAttribute$`
- `$API.ChildSynth.getNumAttributes$`

---

## getChildSynthByIndex

**Signature:** `ChildSynth getChildSynthByIndex(int index)`
**Return Type:** `ScriptObject`
**Call Scope:** init
**Minimal Example:** `var child = {obj}.getChildSynthByIndex(0);`

**Description:**
Returns a ChildSynth reference to the child sound generator at the specified index within this synth's child processor list. Only works if the wrapped synth is a Chain (SynthGroup or SynthChain). If the index is out of range or the synth is not a Chain, returns an invalid ChildSynth. Restricted to `onInit` -- calling at runtime throws a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Zero-based index into the child processor list | 0 to numChildren-1 |

**Pitfalls:**
- [BUG] If the synth is not a Chain type (e.g., a plain SineSynth), the cast to `Chain*` fails and returns an invalid ChildSynth without an error message.

**Cross References:**
- `$API.Synth.getChildSynth$`
- `$API.Synth.getChildSynthByIndex$`
- `$API.ChildSynth.asSampler$`

---

## getCurrentLevel

**Signature:** `float getCurrentLevel(bool leftChannel)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var level = {obj}.getCurrentLevel(true);`

**Description:**
Returns the current peak display level for the specified channel. Pass `true` for the left channel, `false` for the right channel. These are display values (updated at the UI refresh rate), not real-time sample-accurate values.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| leftChannel | Integer | no | true for left channel, false for right channel | -- |

**Cross References:**
None.

---

## getId

**Signature:** `String getId()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var id = {obj}.getId();`

**Description:**
Returns the module ID string of the wrapped synth processor. Returns an empty string if the object is invalid.

**Parameters:**
None.

**Cross References:**
None.

---

## getModulatorChain

**Signature:** `var getModulatorChain(var chainIndex)`
**Return Type:** `ScriptingModulator`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new ScriptingModulator wrapper object (heap allocation).
**Minimal Example:** `var chain = {obj}.getModulatorChain(1);`

**Description:**
Returns a Modulator reference to the modulator chain at the specified index. The chain itself is a Modulator, so the returned handle can be used to control the chain's intensity and bypass state. Chain indices: 1 = GainModulation, 2 = PitchModulation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Chain index (1 = Gain, 2 = Pitch) | Must be a valid child processor index that casts to Modulator |

**Pitfalls:**
- [BUG] Passing an invalid chain index (e.g., 0 for MidiProcessor) may succeed the Modulator cast but give a handle to the wrong chain type. The error message only triggers when the cast to `Modulator*` fails completely.

**Cross References:**
- `$API.ChildSynth.addModulator$`
- `$API.ChildSynth.addGlobalModulator$`
- `$API.ChildSynth.addStaticGlobalModulator$`
- `$API.ChildSynth.setModulationInitialValue$`

---

## getNumAttributes

**Signature:** `int getNumAttributes()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var num = {obj}.getNumAttributes();`

**Description:**
Returns the total number of attributes (parameters) on the wrapped synth processor. Base ModulatorSynth has 4 (Gain, Balance, VoiceLimit, KillFadeTime). Subclasses add more.

**Parameters:**
None.

**Cross References:**
- `$API.ChildSynth.getAttribute$`
- `$API.ChildSynth.setAttribute$`
- `$API.ChildSynth.getAttributeId$`
- `$API.ChildSynth.getAttributeIndex$`

---

## getRoutingMatrix

**Signature:** `var getRoutingMatrix()`
**Return Type:** `ScriptRoutingMatrix`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new ScriptRoutingMatrix wrapper object (heap allocation).
**Minimal Example:** `var rm = {obj}.getRoutingMatrix();`

**Description:**
Returns a RoutingMatrix handle for this synth's channel routing configuration. The matrix allows querying and modifying output channel assignments.

**Parameters:**
None.

**Pitfalls:**
- [BUG] Does not call `checkValidObject()` before creating the ScriptRoutingMatrix. If the synth reference is invalid, creates a RoutingMatrix wrapping nullptr, which will fail on subsequent calls.

**Cross References:**
- `$API.Synth.getRoutingMatrix$`

---

## isBypassed

**Signature:** `bool isBypassed()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var bypassed = {obj}.isBypassed();`

**Description:**
Returns whether the wrapped synth is currently bypassed.

**Parameters:**
None.

**Cross References:**
- `$API.ChildSynth.setBypassed$`

---

## restoreState

**Signature:** `void restoreState(String base64State)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Base64 decoding, ValueTree deserialization, and processor state restoration involve heap allocations and lock acquisition.
**Minimal Example:** `{obj}.restoreState(savedState);`

**Description:**
Restores the processor state from a base64-encoded string previously obtained via `exportState()`. Validates the base64 string by attempting to parse it as a ValueTree before restoring. If the string is invalid, reports a script error "Can't load module state".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| base64State | String | no | Base64-encoded processor state from exportState() | Must be a valid base64-encoded ValueTree |

**Cross References:**
- `$API.ChildSynth.exportState$`

---

## setAttribute

**Signature:** `void setAttribute(int parameterIndex, float newValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Uses ProcessorHelpers::getAttributeNotificationType() which returns the appropriate notification type for the current thread context.
**Minimal Example:** `{obj}.setAttribute({obj}.Gain, 0.5);`

**Description:**
Sets the value of the attribute at the specified parameter index. Uses thread-safe notification dispatching via `ProcessorHelpers::getAttributeNotificationType()`. Standard ModulatorSynth indices: 0 = Gain, 1 = Balance, 2 = VoiceLimit, 3 = KillFadeTime. Use the dynamic constants registered on the ChildSynth instance rather than raw numbers.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| parameterIndex | Integer | no | Index of the attribute to set | Must be a valid parameter index |
| newValue | Double | no | New value for the attribute | Range depends on the specific parameter |

**Cross References:**
- `$API.ChildSynth.getAttribute$`
- `$API.ChildSynth.getAttributeId$`
- `$API.ChildSynth.getAttributeIndex$`
- `$API.ChildSynth.getNumAttributes$`

---

## setBypassed

**Signature:** `void setBypassed(bool shouldBeBypassed)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Sends notification and dispatches ProcessorChangeEvent::Bypassed, which involves message queue operations.
**Minimal Example:** `{obj}.setBypassed(true);`

**Description:**
Sets the bypass state of the wrapped synth. When bypassed, the synth does not produce audio output. Sends a notification and dispatches a `ProcessorChangeEvent::Bypassed` change message.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeBypassed | Integer | no | true to bypass, false to enable | -- |

**Cross References:**
- `$API.ChildSynth.isBypassed$`

---

## setEffectChainOrder

**Signature:** `void setEffectChainOrder(bool doPoly, var slotRange, var chainOrder)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires AudioLock internally via LockHelpers::SafeLock during effect chain reordering.
**Minimal Example:** `{obj}.setEffectChainOrder(false, [0, 3], [2, 1, 0]);`

**Description:**
Changes the processing order of effects within this synth's effect chain. The `slotRange` defines the range of effect slot indices (as `[start, end]`) eligible for reordering. The `chainOrder` is an array of indices within that range specifying the new order. Effects outside the range keep their position. Effects present in the chain but absent from the order array are bypassed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| doPoly | Integer | no | Whether to reorder polyphonic (true) or master (false) effects | Currently ignored -- see pitfall |
| slotRange | Array | no | Two-element array [start, end] defining the dynamic range | Parsed as a Point via ApiHelpers::getPointFromVar |
| chainOrder | Array | no | Array of effect indices within the slot range defining the new order | Indices must be within the slot range |

**Pitfalls:**
- [BUG] The `doPoly` parameter is accepted but hardcoded to `false` when calling `EffectProcessorChain::setFXOrder()`. Only master effect order can be changed regardless of what the caller passes. The implementation reads `fx->setFXOrder(false, ...)` instead of `fx->setFXOrder(doPoly, ...)`.

**Cross References:**
None.

---

## setModulationInitialValue

**Signature:** `void setModulationInitialValue(int chainIndex, float initialValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls ModulatorChain::setInitialValue which may trigger chain recalculation.
**Minimal Example:** `{obj}.setModulationInitialValue(1, 0.5);`

**Description:**
Sets the initial modulation value for the specified modulator chain. This is the default value the chain outputs when no modulators are active. The `chainIndex` maps to the internal chain enum (1 = Gain, 2 = Pitch). Reports a script error if the chain index does not correspond to a valid ModulatorChain.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainIndex | Integer | no | Modulator chain index (1 = Gain, 2 = Pitch) | Must be a valid ModulatorChain index |
| initialValue | Double | no | The initial modulation value | -- |

**Cross References:**
- `$API.ChildSynth.getModulatorChain$`
- `$API.ChildSynth.addModulator$`
