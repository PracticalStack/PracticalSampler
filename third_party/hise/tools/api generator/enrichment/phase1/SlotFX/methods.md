# SlotFX -- Method Analysis

## clear

**Signature:** `undefined clear()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates EmptyFX placeholder (HotswappableProcessor mode) or clears network objects (DspNetwork mode). Both paths involve heap operations and audio lock acquisition.
**Minimal Example:** `{obj}.clear();`

**Description:**
Removes the currently loaded effect and restores the slot to a unity-gain passthrough state. In HotswappableProcessor mode, loads an EmptyFX placeholder and asynchronously deletes the previous effect. In DspNetwork::Holder mode, calls `clearAllNetworks()`. After clearing, the slot passes audio through unchanged with minimal overhead via an internal fast-path that skips all processing when EmptyFX is detected.

**Parameters:**
None.

**Pitfalls:**
- The old effect processor is deleted asynchronously. Any `Effect` handle obtained from a previous `setEffect()` or `getCurrentEffect()` call becomes invalid after `clear()`.

**Cross References:**
- `$API.SlotFX.setEffect$`
- `$API.SlotFX.getCurrentEffect$`
- `$API.SlotFX.getCurrentEffectId$`

## exists

**Signature:** `Integer exists()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var valid = {obj}.exists();`

**Description:**
Returns whether the underlying processor reference is still valid. Checks the internal WeakReference to the processor -- returns true (1) if the processor exists, false (0) if it has been deleted or was never assigned.

**Parameters:**
None.

**Cross References:**
- `$API.SlotFX.getCurrentEffect$`

## getCurrentEffect

**Signature:** `ScriptObject getCurrentEffect()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new ScriptingEffect wrapper object (heap allocation) in HotswappableProcessor mode.
**Minimal Example:** `var fx = {obj}.getCurrentEffect();`

**Description:**
Returns a handle to the effect currently loaded in the slot. In HotswappableProcessor mode, returns an `Effect` object wrapping the active MasterEffectProcessor (including EmptyFX if the slot is cleared). In DspNetwork::Holder mode, returns a `DspNetwork` object for the active network, or undefined if no network is active.

**Parameters:**
None.

**Pitfalls:**
- The return type differs by mode: `Effect` for HotswappableProcessor slots, `DspNetwork` for scriptnode-based slots. Code that assumes one type will fail on the other.

**Cross References:**
- `$API.SlotFX.setEffect$`
- `$API.SlotFX.getCurrentEffectId$`

## getCurrentEffectId

**Signature:** `String getCurrentEffectId()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return involves atomic ref-count operations.
**Minimal Example:** `var id = {obj}.getCurrentEffectId();`

**Description:**
Returns the type name string of the currently loaded effect. In HotswappableProcessor mode, returns the effect type name from the internal effect list (e.g. `"SimpleReverb"`), or `"No Effect"` if the index is out of range. In DspNetwork::Holder mode, returns the active network's ID string.

**Parameters:**
None.

**Cross References:**
- `$API.SlotFX.getCurrentEffect$`
- `$API.SlotFX.setEffect$`
- `$API.SlotFX.getModuleList$`

## getModuleList

**Signature:** `Array getModuleList()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates an Array and String elements.
**Minimal Example:** `var list = {obj}.getModuleList();`

**Description:**
Returns an array of effect type name strings that can be loaded into this slot. In HotswappableProcessor mode, returns the filtered list of allowed MasterEffectProcessor types (excluding polyphonic effects, routing effects, harmonic filters, and nested SlotFX). In DspNetwork::Holder mode, returns the list of available scriptnode network XML files -- but only in the HISE IDE (`USE_BACKEND` builds). In exported plugins, the DspNetwork path returns an empty array.

**Parameters:**
None.

**Pitfalls:**
- In DspNetwork::Holder mode (scriptnode-based slots), this method only works in the HISE IDE. In exported plugins, it silently returns an empty array with no error message.

**Cross References:**
- `$API.SlotFX.setEffect$`

## getParameterProperties

**Signature:** `Array getParameterProperties()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates Array and DynamicObject instances for each parameter.
**Minimal Example:** `var props = {obj}.getParameterProperties();`

**Description:**
Returns an array of objects describing the parameters of the slot's underlying processor. In DspNetwork::Holder mode, iterates the root node's parameters and returns objects with `text` (parameter name), `defaultValue`, and range properties (`min`, `max`, `skew`, `stepSize` in ScriptComponent format). In HotswappableProcessor mode with a HardcodedMasterFX, returns the compiled factory's parameter properties. For a plain SlotFX module, the underlying processor has no parameters of its own, so this returns undefined.

**Parameters:**
None.

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| text | String | The parameter name/ID |
| defaultValue | Double | The parameter's default value |
| min | Double | Range minimum |
| max | Double | Range maximum |
| skew | Double | Skew factor for non-linear ranges |
| stepSize | Double | Step size for discrete parameters |

**Pitfalls:**
- Returns undefined for a plain SlotFX module (which has no parameters of its own). Parameter properties belong to the loaded effect, not the slot container. This method is primarily useful for HardcodedMasterFX and DspNetwork-based slots.

**Cross References:**
- `$API.SlotFX.setEffect$`
- `$API.SlotFX.getCurrentEffect$`
- `$API.SlotFX.getModuleList$`

**Example:**
```javascript:parameter-property-inspection
// Title: Inspecting parameter properties of a loaded network
const var slot = Synth.getSlotFX("MyEffectSlot");

const var params = slot.getParameterProperties();

if (isDefined(params))
{
    for (p in params)
        Console.print(p.text + " [" + p.min + " - " + p.max + "]");
}
```
```json:testMetadata:parameter-property-inspection
{
  "testable": false,
  "skipReason": "Requires a pre-existing SlotFX module with a HardcodedMasterFX or DspNetwork that has parameters"
}
```

## setBypassed

**Disabled:** no-op
**Disabled Reason:** Declared in the C++ header but never implemented or registered (no ADD_API_METHOD macro, no Wrapper entry). Not callable from HiseScript. Use the Effect handle returned by `getCurrentEffect()` or `setEffect()` to control bypass via `Effect.setBypassed()`.

## setEffect

**Signature:** `ScriptObject setEffect(var effectName)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Suspends audio processing, kills voices, acquires processing chain lock. Creates new processor via factory and asynchronously deletes the old one.
**Minimal Example:** `var fx = {obj}.setEffect("SimpleReverb");`

**Description:**
Loads a new effect into the slot by type name. In HotswappableProcessor mode, creates the effect via the processor factory, prepares it (sample rate, block size), swaps it in under audio lock, asynchronously deletes the previous effect, and returns an `Effect` handle. If the loaded effect is a Script FX (JavascriptProcessor), it auto-compiles after loading. If the same effect type is already loaded, the C++ layer returns immediately without reloading.

In DspNetwork::Holder mode, clears all existing networks and loads the named network via `getOrCreate()`. If the requested network is already active, returns the existing `DspNetwork` instance without reloading or clearing. Returns a `DspNetwork` object.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| effectName | String | no | The type name of the effect to load. Must match an entry from `getModuleList()`. | Valid effect type name or network ID |

**Pitfalls:**
- [BUG] If the effect name is not found in HotswappableProcessor mode, the slot is silently cleared (loaded with EmptyFX) and the method returns an `Effect` handle wrapping EmptyFX. The C++ `setEffect()` returns false for invalid names, but the scripting wrapper ignores this return value and always wraps `getCurrentEffect()`.
- The return type differs by mode: `Effect` for HotswappableProcessor slots, `DspNetwork` for scriptnode-based slots.
- In DspNetwork mode, calling `setEffect()` clears ALL previously loaded networks before loading the new one. Any references to previously loaded DspNetwork objects become invalid.

**Cross References:**
- `$API.SlotFX.clear$`
- `$API.SlotFX.getCurrentEffect$`
- `$API.SlotFX.getModuleList$`

## swap

**Signature:** `Integer swap(var otherSlot)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Acquires MainController lock for the atomic exchange.
**Minimal Example:** `var ok = {obj}.swap(otherSlot);`

**Description:**
Atomically exchanges the loaded effects between this slot and another SlotFX instance. Both slots' internal state (effect reference, type index, clear flag) is swapped under the MainController lock. After swapping, both slots send rebuild messages to update their UIs. Only works between HotswappableProcessor-mode SlotFX instances (the classic SlotFX module type).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherSlot | ScriptObject | no | Another SlotFX handle to swap effects with | Must be a valid SlotFX object in HotswappableProcessor mode |

**Pitfalls:**
- Not supported in DspNetwork::Holder mode. Calling swap when the source slot is a scriptnode-based module throws a script error ("Source Slot is invalid").
- [BUG] The C++ swap only works between two `SlotFX` module instances. If the other slot wraps a HardcodedSwappableEffect (which also implements HotswappableProcessor), the swap silently returns false without an error message.

**Cross References:**
- `$API.SlotFX.setEffect$`
- `$API.SlotFX.clear$`
- `$API.SlotFX.getCurrentEffect$`
