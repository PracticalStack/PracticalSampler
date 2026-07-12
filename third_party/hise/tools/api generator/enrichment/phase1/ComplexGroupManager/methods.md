## addGroupEventStartOffset

**Signature:** `undefined addGroupEventStartOffset(Number layerIndex, Number groupIndex, Number offsetSamples)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addGroupEventStartOffset(0, 1, 4096);`

**Description:**
Adds a sample start offset to the voice start data for a specific layer/group combination. When voices matching this layer/group are started on the next noteOn, playback begins from the specified sample position instead of the sample's default start point. Modifies the per-voice StartData stack, which is consumed during voice startup.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIndex | Number | no | Zero-based layer index | 0 to numLayers-1 |
| groupIndex | Number | no | Zero-based group index within the layer | 0 to numGroups-1 |
| offsetSamples | Number | no | Sample start offset in samples | >= 0 |

**Cross References:**
- `$API.ComplexGroupManager.delayGroupEvent$`
- `$API.ComplexGroupManager.fadeInGroupEvent$`
- `$API.ComplexGroupManager.setFixedGroupEventLength$`

## createNoteMap

**Signature:** `undefined createNoteMap(var layerIdOrIndex)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Iterates all samples in the sample map and allocates a VoiceBitMap for lookup storage.
**Minimal Example:** `{obj}.createNoteMap(0);`

**Description:**
Builds a note-to-group mapping for the specified layer by iterating all samples in the current sample map. For each sample with a non-zero, non-IgnoreFlag value in this layer, its note range is recorded in a 128-entry bitmap. The resulting map is stored internally and used by `isNoteNumberMapped()` for fast lookups. Must be called before `isNoteNumberMapped()` for the same layer. Should be called again after sample map changes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIdOrIndex | Number | no | Layer index or string layer ID | Also accepts String layer ID |

**Cross References:**
- `$API.ComplexGroupManager.isNoteNumberMapped$`

## delayGroupEvent

**Signature:** `undefined delayGroupEvent(Number layerIndex, Number groupIndex, Number delayInSamples)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.delayGroupEvent(0, 1, 4410.0);`

**Description:**
Sets a delay in samples for voice starts matching the specified layer/group combination. When voices for this layer/group are started on the next noteOn, their start is delayed by the specified number of samples. Modifies the per-voice StartData stack consumed during voice startup.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIndex | Number | no | Zero-based layer index | 0 to numLayers-1 |
| groupIndex | Number | no | Zero-based group index within the layer | 0 to numGroups-1 |
| delayInSamples | Number | no | Delay time in samples | >= 0 |

**Cross References:**
- `$API.ComplexGroupManager.addGroupEventStartOffset$`
- `$API.ComplexGroupManager.fadeInGroupEvent$`
- `$API.ComplexGroupManager.setFixedGroupEventLength$`

## fadeInGroupEvent

**Signature:** `undefined fadeInGroupEvent(Number layerIndex, Number groupIndex, Number fadeInTimeMs, Number targetGainDb)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.fadeInGroupEvent(0, 1, 100.0, 0.0);`

**Description:**
Sets a fade-in time and target gain for voice starts matching the specified layer/group combination. When voices for this layer/group are started on the next noteOn, they fade in over the specified duration to the target gain level. Internally converts milliseconds to seconds and decibels to a linear gain factor before storing in the StartData stack.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIndex | Number | no | Zero-based layer index | 0 to numLayers-1 |
| groupIndex | Number | no | Zero-based group index within the layer | 0 to numGroups-1 |
| fadeInTimeMs | Number | no | Fade-in duration in milliseconds | >= 0 |
| targetGainDb | Number | no | Target gain in decibels | 0.0 = unity gain, negative = attenuation |

**Cross References:**
- `$API.ComplexGroupManager.fadeOutGroupEvent$`
- `$API.ComplexGroupManager.delayGroupEvent$`
- `$API.ComplexGroupManager.addGroupEventStartOffset$`
- `$API.ComplexGroupManager.setFixedGroupEventLength$`

## fadeOutGroupEvent

**Signature:** `undefined fadeOutGroupEvent(Number layerIndex, Number groupIndex, Number fadeOutTimeMs)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.fadeOutGroupEvent(0, 1, 200.0);`

**Description:**
Fades out all currently playing voices whose sound matches the specified layer/group combination. Unlike the other voice start modification methods (`delayGroupEvent`, `fadeInGroupEvent`, etc.), this operates on already-playing voices rather than setting up data for future voice starts. Iterates the sampler's active voice list, matches voices by their bitmask layer value, and applies a volume fade to silence over the specified duration.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIndex | Number | no | Zero-based layer index | 0 to numLayers-1 |
| groupIndex | Number | no | Zero-based group index within the layer | 0 to numGroups-1 |
| fadeOutTimeMs | Number | no | Fade-out duration in milliseconds | >= 0 |

**Pitfalls:**
- Unlike `delayGroupEvent`, `fadeInGroupEvent`, `addGroupEventStartOffset`, and `setFixedGroupEventLength` which set up start data for future voices, `fadeOutGroupEvent` acts on already-playing voices. Calling it before any matching voices are playing has no effect.

**Cross References:**
- `$API.ComplexGroupManager.fadeInGroupEvent$`
- `$API.ComplexGroupManager.setGroupVolume$`

## getCurrentPeak

**Signature:** `Double getCurrentPeak(Number layerIndex, Number groupIndex, Number eventId)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var peak = {obj}.getCurrentPeak(0, 1, Message.getEventId());`

**Description:**
Returns the current peak volume of the voice matching the specified layer, group, and event ID. Requires `setEnableGainTracking()` to have been called first for the matching layer/group pair; throws a script error if gain tracking was not enabled. Iterates the sampler's active voices to find the voice matching the event ID and layer/group, then returns its peak value from the release sample peak tracking mechanism.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIndex | Number | no | Zero-based layer index | 0 to numLayers-1 |
| groupIndex | Number | no | Zero-based group index within the layer | 0 to numGroups-1 |
| eventId | Number | no | Event ID of the voice to query | Valid event ID from Message.getEventId() |

**Cross References:**
- `$API.ComplexGroupManager.setEnableGainTracking$`

## getLayerIndex

**Signature:** `Integer getLayerIndex(var layerIdOrIndex)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement when a string layer ID is passed -- atomic ref-count operations during string comparison.
**Minimal Example:** `var idx = {obj}.getLayerIndex("Articulation");`

**Description:**
Resolves a layer identifier to its zero-based numeric index. If a String is passed, it is matched against layer IDs defined in the layer configuration. If a Number is passed, it is returned as-is (pass-through). Useful for converting between string-based layer references and the numeric indices accepted by other methods.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIdOrIndex | Number | no | Layer index or string layer ID | Also accepts String layer ID |

## getLayerProperty

**Signature:** `var getLayerProperty(Number layerIndex, String propertyId)`
**Return Type:** `NotUndefined`
**Call Scope:** unsafe
**Call Scope Note:** ValueTree access with String property lookups and type conversion.
**Minimal Example:** `var type = {obj}.getLayerProperty(0, "type");`

**Description:**
Returns the value of a layer configuration property. The return type depends on the property: most return a String or Number, but `tokens` returns a String array and `matrixString` returns an integer array. The property ID is validated against a fixed set of valid identifiers; invalid IDs produce a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIndex | Number | no | Zero-based layer index | 0 to numLayers-1 |
| propertyId | String | no | Property identifier | See Value Descriptions |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "type" | LogicType name ("Custom", "RR", "Keyswitch", "TableFade", "XFade", "Legato", "Release", "Choke") |
| "id" | Layer identifier string used for scripting reference |
| "tokens" | Group names within the layer (returned as String array) |
| "colour" | Display colour for UI representation |
| "folded" | UI folded/collapsed state |
| "ignorable" | Whether samples in this layer can use IgnoreFlag |
| "cached" | Whether samples are pre-filtered into per-group NoteContainers |
| "purgable" | Whether samples can be purged by group value |
| "fader" | XFade curve type ("Linear", "RMS", "Cosine half", "Overlap", "Switch") |
| "slotIndex" | XFade data slot index or MIDI CC number |
| "sourceType" | XFade input source ("Event Data", "Midi CC", "GlobalMod") |
| "matrixString" | Choke group relationship matrix (returned as integer array) |
| "isChromatic" | Whether keyswitch layer includes black keys in range |
| "matchGain" | Whether release layer matches sustain peak on release trigger |
| "accuracy" | Release layer gain matching accuracy (0.0-1.0) |
| "fadeTime" | Fade duration in milliseconds |

**Cross References:**
- `$API.ComplexGroupManager.setLayerProperty$`
- `$API.ComplexGroupManager.getLayerIndex$`

## getNumGroupsInLayer

**Signature:** `Integer getNumGroupsInLayer(Number layerIndex)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var num = {obj}.getNumGroupsInLayer(0);`

**Description:**
Returns the number of groups (tokens) defined in the specified layer. This corresponds to the number of entries in the layer's `tokens` property.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIndex | Number | no | Zero-based layer index | 0 to numLayers-1 |

**Cross References:**
- `$API.ComplexGroupManager.getLayerProperty$`
- `$API.ComplexGroupManager.setActiveGroup$`

## isNoteNumberMapped

**Signature:** `Integer isNoteNumberMapped(Number layerIndex, Number noteNumber)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var mapped = {obj}.isNoteNumberMapped(0, 60);`

**Description:**
Returns whether any samples in the specified layer have a mapping that includes the given MIDI note number. Returns 1 if at least one sample covers this note, 0 otherwise. Requires `createNoteMap()` to have been called first for the same layer; throws a script error if the note map has not been built.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIndex | Number | no | Zero-based layer index | 0 to numLayers-1 |
| noteNumber | Number | no | MIDI note number to check | 0-127 |

**Cross References:**
- `$API.ComplexGroupManager.createNoteMap$`

## registerGroupStartCallback

**Signature:** `undefined registerGroupStartCallback(var layerIdOrIndex, Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a GroupCallback wrapper object and modifies layer post-processor state.
**Minimal Example:** `{obj}.registerGroupStartCallback(0, onGroupStart);`

**Description:**
Registers a callback function that is invoked when a voice starts for the specified layer. The callback receives the zero-based group index as its single argument, or the `IgnoreFlag` constant (255) if the started sample has no assignment in this layer. The callback must be realtime-safe (an `inline function`); non-inline functions are rejected with a script error. When registered on a Custom LogicType layer, the layer is automatically promoted to a post-processor to ensure voice start notifications fire.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIdOrIndex | Number | no | Layer index or string layer ID | Also accepts String layer ID |
| callback | Function | no | Realtime-safe callback for voice start events | Must be an inline function |

**Callback Signature:** callback(groupIndex: int)

**Pitfalls:**
- The callback receives 255 (IgnoreFlag) as the group index when a started sample has no assignment in this layer. Handle this value explicitly rather than treating it as a regular group index.

**Cross References:**
- `$API.ComplexGroupManager.setActiveGroup$`

**Example:**
```javascript:group-start-callback
// Title: Registering a voice start callback for a Custom layer
const var Sampler1 = Synth.getChildSynth("Sampler1");
const var cgm = Sampler1.getComplexGroupManager();

reg lastStartedGroup = -1;

inline function onGroupStart(groupIndex)
{
    if (groupIndex != cgm.IgnoreFlag)
        lastStartedGroup = groupIndex;
}

cgm.registerGroupStartCallback(0, onGroupStart);
```
```json:testMetadata:group-start-callback
{
  "testable": false,
  "skipReason": "Requires a loaded sample map with ComplexGroupManager layers and MIDI input to trigger voice starts"
}
```

## setActiveGroup

**Signature:** `undefined setActiveGroup(Number layerIndex, Number groupIndex)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setActiveGroup(0, 2);`

**Description:**
Sets the active group filter for the specified layer. On the next noteOn, only samples matching this group value in the given layer will be considered for voice start. The group index is zero-based and converted to one-based internally. Rejects `IgnoreFlag` (255) with a script error -- use a valid group index instead. Uses asynchronous notification for thread-safe state updates when called from the scripting thread.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIndex | Number | no | Zero-based layer index | 0 to numLayers-1 |
| groupIndex | Number | no | Zero-based group index to activate | 0 to numGroups-1; IgnoreFlag (255) rejected |

**Cross References:**
- `$API.ComplexGroupManager.getNumGroupsInLayer$`
- `$API.ComplexGroupManager.registerGroupStartCallback$`

## setEnableGainTracking

**Signature:** `undefined setEnableGainTracking(var layerIdOrIndex, Number groupIndex, Integer shouldBeActive)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Iterates all samples in the sample map and modifies per-sample release tracking state.
**Minimal Example:** `{obj}.setEnableGainTracking(0, 1, 1);`

**Description:**
Enables or disables peak volume tracking for a specific layer/group combination. When enabled, repurposes the release sample peak tracking mechanism to monitor the current peak of playing voices in this layer/group. After enabling, use `getCurrentPeak()` to read the tracked value for a specific voice by event ID. Rejects `IgnoreFlag` for the group index. Iterates all samples matching the layer/group and sets their release sample tracking flag.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIdOrIndex | Number | no | Layer index or string layer ID | Also accepts String layer ID |
| groupIndex | Number | no | Zero-based group index within the layer | 0 to numGroups-1; IgnoreFlag rejected |
| shouldBeActive | Integer | no | Enable (1) or disable (0) gain tracking | 0 or 1 |

**Cross References:**
- `$API.ComplexGroupManager.getCurrentPeak$`

## setFixedGroupEventLength

**Signature:** `undefined setFixedGroupEventLength(Number layerIndex, Number groupIndex, Number numSamplesToPlayBeforeFadeout)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setFixedGroupEventLength(0, 1, 44100.0);`

**Description:**
Sets a fixed playback length in samples for voice starts matching the specified layer/group combination. When voices for this layer/group are started on the next noteOn, they are automatically faded out after the specified number of samples regardless of note-off messages. Modifies the per-voice StartData stack consumed during voice startup.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIndex | Number | no | Zero-based layer index | 0 to numLayers-1 |
| groupIndex | Number | no | Zero-based group index within the layer | 0 to numGroups-1 |
| numSamplesToPlayBeforeFadeout | Number | no | Fixed playback length in samples before fadeout | > 0 |

**Cross References:**
- `$API.ComplexGroupManager.delayGroupEvent$`
- `$API.ComplexGroupManager.fadeInGroupEvent$`
- `$API.ComplexGroupManager.addGroupEventStartOffset$`

## setGroupVolume

**Signature:** `undefined setGroupVolume(Number layerIndex, Number groupIndex, Number gainFactor)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setGroupVolume(0, 1, 0.5);`

**Description:**
Sets the per-group gain factor for a specific layer/group combination. The gain factor is applied as a linear multiplier to the voice output. Only works with layers that use the Custom LogicType. On non-Custom layers, the internal cast to CustomLayer fails and the call has no effect without producing any error message.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIndex | Number | no | Zero-based layer index | 0 to numLayers-1 |
| groupIndex | Number | no | Zero-based group index within the layer | 0 to numGroups-1 |
| gainFactor | Number | no | Linear gain multiplier | 0.0 = silence, 1.0 = unity gain |

**Pitfalls:**
- [BUG] Silently does nothing when called on a non-Custom LogicType layer. The internal cast to CustomLayer returns null and the gain value is never applied. No error message is produced.

**Cross References:**
- `$API.ComplexGroupManager.fadeOutGroupEvent$`
- `$API.ComplexGroupManager.getLayerProperty$`

## setLayerProperty

**Signature:** `undefined setLayerProperty(Number layerIndex, String propertyId, var value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** ValueTree modification with async listener notifications and type conversion.
**Minimal Example:** `{obj}.setLayerProperty(0, "fadeTime", 50.0);`

**Description:**
Sets a layer configuration property value. Validates the property ID against a fixed set of valid identifiers; invalid IDs produce a script error. Performs type conversion for special properties: `tokens` accepts a String array (stored as comma-separated text), `matrixString` accepts an integer array (stored as encoded string). Changes to structural properties like `tokens` trigger a full layer and bitmask rebuild.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| layerIndex | Number | no | Zero-based layer index | 0 to numLayers-1 |
| propertyId | String | no | Property identifier | See Value Descriptions |
| value | NotUndefined | no | Property value (type depends on property) | Must match expected type for the property |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "type" | LogicType name ("Custom", "RR", "Keyswitch", "TableFade", "XFade", "Legato", "Release", "Choke") |
| "id" | Layer identifier string |
| "tokens" | Group names (pass as String array; stored as comma-separated text) |
| "colour" | Display colour integer |
| "folded" | UI folded state (boolean as integer) |
| "ignorable" | Whether samples can use IgnoreFlag (boolean as integer) |
| "cached" | Enable per-group NoteContainer pre-filtering (boolean as integer) |
| "purgable" | Enable sample purging by group value (boolean as integer) |
| "fader" | XFade curve type ("Linear", "RMS", "Cosine half", "Overlap", "Switch") |
| "slotIndex" | XFade data slot index or MIDI CC number (integer) |
| "sourceType" | XFade input source ("Event Data", "Midi CC", "GlobalMod") |
| "matrixString" | Choke matrix (pass as integer array; stored as encoded string) |
| "isChromatic" | Include black keys in keyswitch range (boolean as integer) |
| "matchGain" | Match sustain peak on release (boolean as integer) |
| "accuracy" | Release gain match accuracy (double, 0.0-1.0) |
| "fadeTime" | Fade duration in milliseconds (double) |

**Cross References:**
- `$API.ComplexGroupManager.getLayerProperty$`
- `$API.ComplexGroupManager.getLayerIndex$`
