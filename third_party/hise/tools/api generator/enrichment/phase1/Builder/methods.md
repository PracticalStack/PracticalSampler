# Builder -- Method Documentation

## clear

**Signature:** `void clear()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires MessageManagerLock, performs heap deallocation via raw::Builder::remove, suspends global dispatch. In backend IDE, blocks for 500ms for UI coordination.
**Minimal Example:** `{obj}.clear();`

**Description:**
Removes all modules from the MainSynthChain except the calling script processor itself. Suspends the global dispatch system during demolition to prevent notification storms. After removal, cleans up unconnected global routing cables via GlobalRoutingManager::removeUnconnectedSlots. Sets the internal flushed flag to false, requiring a subsequent `flush()` call.

**Parameters:**
None.

**Pitfalls:**
- Silently skips execution when running on the sample loading thread (during project load). A console message is logged but the method returns without error, which may cause subsequent `create()` calls to add modules to an existing tree rather than a clean slate.

**Cross References:**
- `$API.Builder.flush$`
- `$API.Builder.create$`

## clearChildren

**Signature:** `undefined clearChildren(int buildIndex, int chainIndex)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires MessageManagerLock for each processor removal. Performs heap deallocation via Chain handler remove.
**Minimal Example:** `{obj}.clearChildren(0, {obj}.ChainIndexes.FX);`

**Description:**
Removes all child processors from a specific chain of the module at `buildIndex`. Use `chainIndex` values from the `ChainIndexes` constant -- pass `ChainIndexes.Direct` (-1) to treat the module itself as the chain (for removing sound generators from a container). The C++ implementation returns the number of removed processors, but the script wrapper discards this value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| buildIndex | Integer | no | Index of the parent module in the Builder's internal array | Must be a valid build index |
| chainIndex | Integer | no | Chain to clear: -1 (Direct), 0 (Midi), 1 (Gain), 2 (Pitch), 3 (FX) | Use ChainIndexes constants |

**Pitfalls:**
- [BUG] Unlike `clear()`, this method has no self-preservation check. If the calling script processor is in the targeted chain, it will be removed, potentially causing undefined behavior or a silent crash.

**Cross References:**
- `$API.Builder.clear$`
- `$API.Builder.create$`
- `$API.Builder.flush$`

## connectToScript

**Signature:** `undefined connectToScript(int buildIndex, String relativePath)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Involves file I/O via JavascriptProcessor::setConnectedFile.
**Minimal Example:** `{obj}.connectToScript(midiIdx, "midiProcessor.js");`

**Description:**
Links the module at `buildIndex` to an external script file. Only works if the target module is a script processor (JavascriptMidiProcessor or similar JavascriptProcessor subclass). The `relativePath` is relative to the project's Scripts folder. The C++ returns a bool indicating success, but the script wrapper discards it.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| buildIndex | Integer | no | Index of the target module in the Builder's internal array | Must point to a JavascriptProcessor |
| relativePath | String | no | Path to the script file, relative to the project Scripts folder | Must be a valid .js file path |

**Pitfalls:**
- [BUG] Silently does nothing if the module at `buildIndex` is not a JavascriptProcessor. No error message is reported and the void wrapper discards the false return value, so the user has no way to detect failure.

**Cross References:**
- `$API.Builder.create$`
- `$API.Builder.get$`

## create

**Signature:** `int create(String type, String id, int rootBuildIndex, int chainIndex)`
**Return Type:** `Integer`
**Call Scope:** init
**Call Scope Note:** Enforced by interfaceCreationAllowed() check -- throws script error if called after onInit.
**Minimal Example:** `var idx = {obj}.create({obj}.Effects.SimpleGain, "MyGain", 0, {obj}.ChainIndexes.FX);`

**Description:**
Creates a new module of the specified type and adds it to the chain at `chainIndex` of the parent module at `rootBuildIndex`. Returns the new module's build index. If a processor with the given `id` already exists under the parent, returns the existing module's index without creating a duplicate (idempotent behavior). The `type` must be a type string from one of the dynamic constants: `SoundGenerators`, `Modulators`, `Effects`, or `MidiProcessors`. Returns -1 on failure.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| type | String | no | Module type from dynamic constants (e.g. `b.Effects.SimpleGain`) | Must match a registered factory type |
| id | String | no | Unique processor ID for the new module | Should be unique within the parent scope |
| rootBuildIndex | Integer | no | Build index of the parent module | 0 = MainSynthChain |
| chainIndex | Integer | no | Target chain: -1 (Direct), 0 (Midi), 1 (Gain), 2 (Pitch), 3 (FX) | Use ChainIndexes constants |

**Pitfalls:**
- Idempotent by design: if a processor with the given ID already exists under the parent, the existing module is silently reused and its build index returned. No warning is issued, which can mask accidental ID collisions.

**Cross References:**
- `$API.Builder.flush$`
- `$API.Builder.get$`
- `$API.Builder.setAttributes$`
- `$API.Builder.clearChildren$`

**Example:**
```javascript:builder-create-chain
// Title: Creating a synth with effect chain
const var b = Synth.createBuilder();
b.clear();
var synthIdx = b.create(b.SoundGenerators.SineSynth, "MySine", 0, b.ChainIndexes.Direct);
var gainIdx = b.create(b.Effects.SimpleGain, "MyGain", synthIdx, b.ChainIndexes.FX);
b.setAttributes(gainIdx, {"Gain": -6.0});
b.flush();

Console.print(Synth.getChildSynth("MySine").getId());
Console.print(Synth.getEffect("MyGain").getAttribute(0));
```
```json:testMetadata:builder-create-chain
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["MySine", "-6"]}
}
```

## flush

**Signature:** `void flush()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls MessageManager::callAsync which involves heap allocation for the lambda capture.
**Minimal Example:** `{obj}.flush();`

**Description:**
Finalizes all pending module tree changes by notifying the UI and patch browser. Schedules an async message-thread callback that clears the `isRebuilding` flag, sends a rebuild message to the synth chain, and dispatches a `RebuildModuleList` event. Does nothing if already flushed. Must be called after `create()` or `clear()` operations -- the Builder's destructor emits a console warning if this is forgotten.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.Builder.create$`
- `$API.Builder.clear$`
- `$API.Builder.clearChildren$`

## get

**Signature:** `var get(int buildIndex, String interfaceType)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new scripting wrapper object on the heap.
**Minimal Example:** `var fx = {obj}.get(fxIdx, {obj}.InterfaceTypes.Effect);`

**Description:**
Returns a typed scripting wrapper for the module at the given build index. The `interfaceType` string must match one of the class names from the `InterfaceTypes` constant. The module must also be dynamically castable to the corresponding C++ type. This is the Builder's equivalent of `Synth.getEffect()` or `Synth.getModulator()` -- addressed by build index instead of name, with explicit type selection. Returns undefined on failure.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| buildIndex | Integer | no | Index of the module in the Builder's internal array | Must be a valid build index |
| interfaceType | String | no | Script wrapper type from the InterfaceTypes constant | Must match the module's actual type |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "MidiProcessor" | Returns a ScriptingMidiProcessor wrapper for MIDI processor modules |
| "Modulator" | Returns a ScriptingModulator wrapper for modulator modules |
| "ChildSynth" | Returns a ScriptingSynth wrapper for sound generator modules |
| "Effect" | Returns a ScriptingEffect wrapper for effect processor modules |
| "AudioSampleProcessor" | Returns a ScriptingAudioSampleProcessor wrapper for audio file processors |
| "SliderPackProcessor" | Returns a ScriptSliderPackProcessor wrapper for modules with slider pack data |
| "TableProcessor" | Returns a ScriptingTableProcessor wrapper for modules with table data |
| "Sampler" | Returns a Sampler wrapper for ModulatorSampler modules |
| "MidiPlayer" | Returns a ScriptedMidiPlayer wrapper for MIDI player modules |
| "RoutingMatrix" | Returns a ScriptRoutingMatrix wrapper for accessing the module's routing matrix |
| "SlotFX" | Returns a ScriptingSlotFX wrapper for slot effect modules |

**Pitfalls:**
- [BUG] Returns undefined silently if the build index is invalid or the module reference has been released. Unlike `clearChildren()` or `getExisting()`, no error message is reported.
- [BUG] Returns undefined silently if the `interfaceType` string does not match the module's actual C++ type. No error or warning is produced for the mismatch.

**Cross References:**
- `$API.Builder.create$`
- `$API.Builder.getExisting$`

**Example:**
```javascript:builder-get-typed-ref
// Title: Getting typed references after creation
const var b = Synth.createBuilder();
b.clear();
var synthIdx = b.create(b.SoundGenerators.SineSynth, "TestSine", 0, b.ChainIndexes.Direct);
var gainIdx = b.create(b.Effects.SimpleGain, "TestGain", synthIdx, b.ChainIndexes.FX);
b.flush();

var synth = b.get(synthIdx, b.InterfaceTypes.ChildSynth);
var gain = b.get(gainIdx, b.InterfaceTypes.Effect);
Console.print(synth.getId());
Console.print(gain.getId());
```
```json:testMetadata:builder-get-typed-ref
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["TestSine", "TestGain"]}
}
```

## getExisting

**Signature:** `int getExisting(String processorId)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Traverses the processor tree with string comparisons. May grow the internal array (heap allocation).
**Minimal Example:** `var idx = {obj}.getExisting("MySynth");`

**Description:**
Registers a pre-existing processor (not created by this Builder instance) into the Builder's internal tracking array and returns its build index. If the processor is already tracked, returns the existing index without adding a duplicate. Searches the entire module tree starting from the MainSynthChain using `ProcessorHelpers::getFirstProcessorWithName`. Reports a script error if no processor with the given ID is found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| processorId | String | no | The ID of the processor to find and register | Must match an existing processor's ID |

**Pitfalls:**
None.

**Cross References:**
- `$API.Builder.get$`
- `$API.Builder.create$`

## setAttributes

**Signature:** `void setAttributes(int buildIndex, var attributeValues)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls setAttribute on the processor and sends async change notification via sendOtherChangeMessage.
**Minimal Example:** `{obj}.setAttributes(synthIdx, {"OctaveTranspose": 5, "Detune": 0.1});`

**Description:**
Sets multiple attributes on the module at `buildIndex` in a single call. The `attributeValues` parameter is a JSON object mapping attribute names (as Identifiers) to numeric values. Each value is cast to float and sanitized via FloatSanitizers. Individual attributes are set with `dontSendNotification`, and a single batch async notification is sent after all attributes are applied. Reports a script error and stops processing on the first unrecognized attribute name.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| buildIndex | Integer | no | Index of the target module in the Builder's internal array | Must be a valid build index |
| attributeValues | JSON | no | Object mapping attribute names to numeric values | Keys must match the module's parameter identifiers |

**Pitfalls:**
- Stops on first unrecognized attribute: if the JSON object contains an invalid attribute name, the method reports a script error and breaks out of the loop. Any subsequent valid attributes in the object are silently skipped.
- [BUG] All values are cast to float. Non-numeric values (strings, objects) will silently become 0.0 via the C++ `(float)` cast, with no validation or warning.

**Cross References:**
- `$API.Builder.create$`
- `$API.Builder.get$`
- `$API.Builder.flush$`

**Example:**
```javascript:builder-set-attributes
// Title: Configuring module parameters after creation
const var b = Synth.createBuilder();
b.clear();
var synthIdx = b.create(b.SoundGenerators.SineSynth, "AttrSine", 0, b.ChainIndexes.Direct);
b.setAttributes(synthIdx, {"OctaveTranspose": 3, "SemiTones": 7});
b.flush();

const var sine = Synth.getChildSynth("AttrSine");
Console.print(sine.getAttribute(sine.OctaveTranspose));
Console.print(sine.getAttribute(sine.SemiTones));
```
```json:testMetadata:builder-set-attributes
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["3", "7"]}
}
```
