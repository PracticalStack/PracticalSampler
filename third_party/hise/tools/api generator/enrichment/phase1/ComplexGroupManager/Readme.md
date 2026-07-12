# ComplexGroupManager -- Class Analysis

## Brief
Multi-dimensional bitmask-based sample group controller for advanced layer, crossfade, and articulation management.

## Purpose
The ComplexGroupManager provides scripting control over a multi-layer sample organization system that replaces the Sampler's simple round-robin group management. It uses a 64-bit bitmask to encode multiple independent dimensions (layers) into each sample, allowing simultaneous filtering by articulation, round-robin, crossfade position, release triggers, legato intervals, and choke groups. Each layer has a LogicType that determines its automatic behavior during note processing. The scripting API exposes per-layer/per-group control over voice start parameters (delay, fade-in, start offset, fixed length), volume, gain tracking, and voice start callbacks.

## Details

### Layer System Architecture

Each ComplexGroupManager contains an ordered list of layers stored as a ValueTree. Each layer occupies a contiguous range of bits in a 64-bit bitmask. Layers are defined by:
- An **id** (string identifier for scripting access)
- A **LogicType** that determines automatic behavior
- A **tokens** list (comma-separated group names within the layer)
- **Flags** controlling caching, purging, ignorability, and processing hooks

### LogicType Behaviors

| LogicType | Description | Auto-Behavior |
|-----------|-------------|---------------|
| Custom | Scriptable layer with optional gain modulation | Voice start callbacks, per-group volume |
| RoundRobin | Automatic round-robin cycling | Advances group on each noteOn |
| Keyswitch | MIDI-key-driven articulation switching | Switches group when keyswitch note is played |
| TableFade | Legacy crossfade using sampler's crossfade tables | Integrates with ModulatorSampler crossfade system |
| XFade | Dynamic crossfade with multiple input sources | Per-voice gain modulation from EventData/MidiCC/GlobalMod |
| LegatoInterval | Legato transition samples | Phase-locked transitions with zero-crossing alignment |
| ReleaseTrigger | Sustain/release sample pairs | Auto note-off handling with gain matching |
| Choke | Mutual voice cancellation groups | Fades out voices in same choke group |

### Zero-Based Indexing Convention

The scripting API uses zero-based group indices. Internally, the bitmask system is one-based (0 means "unassigned"). The conversion is automatic -- scripts pass 0 for the first group, 1 for the second, etc. The special constant `IgnoreFlag` (255) bypasses this conversion and means "this layer does not apply."

### Layer Properties

Layer configuration is accessed through `setLayerProperty()` / `getLayerProperty()` with string property IDs:

| Property | Type | Applies To |
|----------|------|------------|
| type | String | All (LogicType name) |
| id | String | All (layer identifier) |
| tokens | String[] | All (group names within layer) |
| colour | int | All (UI display colour) |
| ignorable | bool | All (samples can have IgnoreFlag) |
| cached | bool | All (pre-filter into NoteContainers) |
| purgable | bool | All (samples can be purged by group) |
| fader | String | XFade (curve type) |
| slotIndex | int | XFade (data slot or CC number) |
| sourceType | String | XFade (input source) |
| matrixString | int[] | Choke (group relationships) |
| isChromatic | bool | Keyswitch (include black keys) |
| matchGain | bool | Release (match sustain peak) |
| accuracy | double | Release (gain match accuracy 0-1) |
| fadeTime | double | Multiple (fade time in ms) |

Note: `matrixString` is stored internally as an encoded string but converted to/from an integer array in the scripting API. `tokens` is stored as comma-separated text but converted to/from a string array.

### Voice Start Modification

The methods `delayGroupEvent`, `fadeInGroupEvent`, `setFixedGroupEventLength`, and `addGroupEventStartOffset` modify how voices are started for a specific layer/group combination. These set up per-voice start data (delay, offset, fade-in, fixed playback length) that is applied when the sampler starts voices on the next noteOn. In contrast, `fadeOutGroupEvent` operates on already-playing voices.

### Gain Tracking

`setEnableGainTracking` repurposes the release sample peak tracking mechanism to monitor the current peak volume of playing voices. After enabling, `getCurrentPeak` returns the peak value for a specific voice identified by event ID, layer, and group.

## obtainedVia
`Sampler.getComplexGroupManager()`

## minimalObjectToken
cgm

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| IgnoreFlag | 255 | int | Special value indicating a layer does not apply to a sample or should be bypassed in filtering | LayerControl |

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `cgm.setGroupVolume(0, 1, 0.5);` on a non-Custom layer | `cgm.setGroupVolume(0, 1, 0.5);` on a Custom layer | `setGroupVolume` only works with Custom LogicType layers. On other layer types, the internal cast to CustomLayer fails silently. |
| `cgm.isNoteNumberMapped(0, 60);` without createNoteMap | `cgm.createNoteMap(0); cgm.isNoteNumberMapped(0, 60);` | `isNoteNumberMapped` requires `createNoteMap()` to be called first for the given layer, otherwise a script error is thrown. |
| `cgm.getCurrentPeak(0, 1, eventId);` without enabling tracking | `cgm.setEnableGainTracking(0, 1, true); // then later: cgm.getCurrentPeak(0, 1, eventId);` | `getCurrentPeak` requires `setEnableGainTracking` to be called first for the matching layer/group pair. |
| `cgm.setActiveGroup(0, cgm.IgnoreFlag);` | `cgm.setActiveGroup(0, 0);` | `setActiveGroup` rejects IgnoreFlag with a script error. Use a valid zero-based group index. |

## codeExample
```javascript
// Get the ComplexGroupManager from a Sampler
const var cgm = Sampler.getComplexGroupManager();

// Query layer structure
const var numGroups = cgm.getNumGroupsInLayer(0);

// Set the active group for the first layer
cgm.setActiveGroup(0, 0);
```

## Alternatives
- `Sampler` -- handles sample map loading and basic single-dimension RR group switching; ComplexGroupManager provides multi-dimensional per-layer/per-group control.

## Related Preprocessors
`USE_BACKEND` (realtime safety check in registerGroupStartCallback, XFade UI updates).

## Diagnostic Ideas
Reviewed: Yes
Count: 1
- ComplexGroupManager.isNoteNumberMapped -- timeline dependency on createNoteMap (script error if not called first)
