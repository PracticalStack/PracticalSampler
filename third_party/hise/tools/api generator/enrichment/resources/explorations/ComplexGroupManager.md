# ComplexGroupManager -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` (ComplexGroupManager entry)
- `enrichment/resources/survey/class_survey.md` (prerequisite row 5: Sampler -> ComplexGroupManager)
- `enrichment/phase1/Sampler/Readme.md` (prerequisite context)
- `enrichment/base/ComplexGroupManager.json` (16 API methods)

## Prerequisite Context: Sampler

The Sampler class provides scripting access to ModulatorSampler. Its round-robin group management uses `enableRoundRobin(false)` + `setActiveGroup()` for simple single-dimension group control. ComplexGroupManager replaces this with a multi-dimensional bitmask-based system where samples can be organized across multiple independent layers simultaneously.

Key Sampler concepts relevant here:
- ModulatorSampler is the underlying C++ sampler engine
- Round-robin groups are one-based (RRGroup property)
- `SampleMap::Listener` is used for change notifications
- Voice iteration via `sampler->activeVoices`

---

## Class Declaration

### Scripting Wrapper

**File:** `hi_scripting/scripting/api/ScriptingApiObjects.h:3288`

```cpp
struct ScriptingComplexGroupManager: public ConstScriptingObject
```

- Extends `ConstScriptingObject` (not `DynamicScriptingObject`)
- Static class name: `"ComplexGroupManager"` via `RETURN_STATIC_IDENTIFIER`
- Holds a `WeakReference<ModulatorSampler> sampler`
- Private `getManager()` method returns `sampler->getComplexGroupManager()`
- Private `getLayerIndexInternal()` resolves `var` to layer index (accepts String ID or int index)

### Core Engine Class

**File:** `hi_core/hi_sampler/sampler/ComplexGroupManager.h:236`

```cpp
struct ComplexGroupManager: public ModulatorSynth::SoundCollectorBase,
                            public SampleMap::Listener
```

- Implements `SoundCollectorBase` -- the interface that ModulatorSynth uses to decide which sounds to start on noteOn
- Implements `SampleMap::Listener` -- reacts to sample map changes, sample property changes, sample amount changes
- The sampler stores it as `soundCollector` and retrieves it via `dynamic_cast<ComplexGroupManager*>(soundCollector.get())`

---

## Factory / obtainedVia

Created by `Sampler.getComplexGroupManager()`:

```cpp
// ScriptingApi.cpp:5231
var ScriptingApi::Sampler::getComplexGroupManager()
{
    return new ScriptingObjects::ScriptingComplexGroupManager(
        getScriptProcessor(), dynamic_cast<ModulatorSampler*>(sampler.get()));
}
```

The underlying `ComplexGroupManager` is created when `ModulatorSampler::setUseComplexGroupManager(true)` is called. It is stored as the sampler's `soundCollector`. The sampler retrieves it via:

```cpp
ComplexGroupManager* getComplexGroupManager() const
{
    return dynamic_cast<ComplexGroupManager*>(soundCollector.get());
}
```

Returns `nullptr` if the sampler does not use complex group management.

---

## Constructor

**File:** `ScriptingApiObjects.cpp:10760`

```cpp
ScriptingComplexGroupManager(ProcessorWithScriptingContent* pwsc, ModulatorSampler* sampler_) :
    ConstScriptingObject(pwsc, 1),  // 1 = numConstants
    sampler(sampler_)
{
    addConstant("IgnoreFlag", (int)ComplexGroupManager::IgnoreFlag);  // 0xFF = 255

    ADD_API_METHOD_2(getLayerProperty);
    ADD_API_METHOD_3(setLayerProperty);
    ADD_API_METHOD_1(getLayerIndex);
    ADD_API_METHOD_1(getNumGroupsInLayer);
    ADD_API_METHOD_2(setActiveGroup);
    ADD_API_METHOD_1(createNoteMap);
    ADD_API_METHOD_2(isNoteNumberMapped);
    ADD_API_METHOD_3(setEnableGainTracking);
    ADD_API_METHOD_3(getCurrentPeak);
    ADD_API_METHOD_2(registerGroupStartCallback);
    ADD_API_METHOD_3(delayGroupEvent);
    ADD_API_METHOD_4(fadeInGroupEvent);
    ADD_API_METHOD_3(setFixedGroupEventLength);
    ADD_API_METHOD_3(addGroupEventStartOffset);
    ADD_API_METHOD_3(fadeOutGroupEvent);
    ADD_API_METHOD_3(setGroupVolume);
}
```

All methods use plain `ADD_API_METHOD_N` (no typed variants). The Wrapper struct uses `API_VOID_METHOD_WRAPPER_N` and `API_METHOD_WRAPPER_N`.

---

## Constants

| Name | Value | Type | Description |
|------|-------|------|-------------|
| IgnoreFlag | 255 (0xFF) | int | Special value meaning "this layer does not apply to this sample" or "ignore this layer's filter" |

The `IgnoreFlag` is defined in the core class:
```cpp
static constexpr uint8 IgnoreFlag = 0xFF;
```

---

## Enums and String Constant Tables

### LogicType Enum

**File:** `ComplexGroupManager.h:256`

```cpp
enum class LogicType
{
    Undefined,
    Custom,
    RoundRobin,
    Keyswitch,
    TableFade,
    XFade,
    LegatoInterval,
    ReleaseTrigger,
    Choke,
    numLogicTypes
};
```

String names (from `getLogicTypeNames()`):
```
{ "Ignore", "Custom", "RR", "Keyswitch", "TableFade", "XFade", "Legato", "Release", "Choke" }
```

### Flags Enum

**File:** `ComplexGroupManager.h:245`

```cpp
enum Flags
{
    FlagIgnorable          = 0x01,  // Layer value can be IgnoreFlag (sample applies to all values)
    FlagCached             = 0x02,  // Pre-filter samples into NoteContainers per value
    FlagXFade              = 0x04,  // Layer uses crossfade (set automatically for TableFade/XFade)
    FlagPurgable           = 0x08,  // Samples can be purged by layer value
    FlagProcessHiseEvent   = 0x10,  // Layer's handleHiseEvent() is called on MIDI events
    FlagProcessPostSounds  = 0x20,  // Layer's postVoiceStart() is called after sound collection
    FlagProcessModulation  = 0x40   // Layer provides per-voice gain modulation
};
```

### Default Flags per LogicType

From `Helpers::getDefaultValue()`:

| LogicType | Ignorable | Cached | Purgable |
|-----------|-----------|--------|----------|
| RoundRobin | No | Unspecified | No |
| Keyswitch | No | Yes | Yes |
| TableFade | No | No | No |
| XFade | No | No | No |
| LegatoInterval | Yes | No | No |
| ReleaseTrigger | No | Yes | Yes |
| Choke | Yes | No | No |

### XFade FaderTypes

```cpp
enum class FaderTypes { Linear, RMS, CosineHalf, Overlap, Switch, numFaderTypes };
```

String names: `{ "Linear", "RMS", "Cosine half", "Overlap", "Switch" }`

### XFade SourceTypes

```cpp
enum class SourceTypes { EventData, MidiCC, GlobalMod, numSourceTypes };
```

String names: `{ "Event Data", "Midi CC", "GlobalMod" }`

---

## Layer Properties (Valid Property IDs)

From `Helpers::isValidId()` (`ComplexGroupManager.cpp:1424`):

| Property ID | Description | Used By |
|-------------|-------------|---------|
| type | LogicType name string | All layers |
| colour | Display colour | All layers (UI) |
| id | Layer identifier | All layers |
| folded | UI folded state | All layers (UI) |
| tokens | Comma-separated group names | All layers |
| ignorable | Whether samples can be ignored | All layers |
| cached | Whether to pre-cache sample groups | All layers |
| purgable | Whether samples can be purged | All layers |
| fader | Fader type name | XFade layers |
| slotIndex | Data slot or CC number | XFade layers |
| sourceType | Input source name | XFade layers |
| matrixString | Encoded choke matrix | Choke layers |
| isChromatic | Include black keys in keyswitch range | Keyswitch layers |
| matchGain | Match sustain gain on release | ReleaseTrigger layers |
| accuracy | Release gain matching accuracy 0-1 | ReleaseTrigger layers |
| fadeTime | Fade time in ms | Multiple layer types |

---

## groupIds Namespace

**File:** `ComplexGroupManager.h:41`

All layer property identifiers are defined as `DECLARE_ID` in the `groupIds` namespace:
`Layers`, `Layer`, `type`, `colour`, `id`, `folded`, `tokens`, `ignorable`, `cached`, `purgable`, `fader`, `slotIndex`, `sourceType`, `matrixString`, `isChromatic`, `matchGain`, `accuracy`, `fadeTime`, `groupStart`, `gainMod`.

Note: `groupStart` and `gainMod` are used by CustomLayer but are NOT in the `isValidId()` list -- they can only be set through the UI, not through `setLayerProperty()`.

---

## Zero-Based to One-Based Index Conversion

The scripting API uses zero-based group indices, but the internal bitmask system uses one-based values (0 means "unassigned"). The conversion is handled by `bumpGroupIndexFromZeroBased()`:

```cpp
static uint8 bumpGroupIndexFromZeroBased(int groupIndex)
{
    return uint8(groupIndex + ((uint8)groupIndex != ComplexGroupManager::IgnoreFlag));
}
```

This adds 1 to all values EXCEPT IgnoreFlag (0xFF), which stays as 0xFF. So:
- Script passes 0 -> internal gets 1
- Script passes 1 -> internal gets 2
- Script passes IgnoreFlag (255) -> internal gets 255 (unchanged)

The GroupCallback reverses this for voice start notifications:
```cpp
void onVoiceStart(uint8 groupValue) final
{
    args = groupValue == ComplexGroupManager::IgnoreFlag 
           ? (int)ComplexGroupManager::IgnoreFlag 
           : (int)(groupValue - 1);
    callback.callSync(&args, 1, nullptr);
}
```

---

## Bitmask Architecture

The core of ComplexGroupManager is a 64-bit bitmask system (`uint64 Bitmask`) that encodes multiple layer values into a single integer per sample.

### Bit Layout

Each layer occupies a contiguous range of bits. The number of bits per layer is `ceil(log2(numItems + 1))` (one-based to avoid zero). If a layer is ignorable, it gets one additional bit for the ignore flag.

Example with 3 layers:
- Layer "articulation" (4 items): bits 0-2 (3 bits for values 0-5)
- Layer "RR" (3 items): bits 3-4 (2 bits for values 0-4)
- Layer "legato" (ignorable, 2 items): bits 5-6 (2 bits for values) + bit 7 (ignore flag)

### SynthSoundWithBitmask

Base class for sounds that carry a bitmask. `ModulatorSamplerSound` inherits from this. Key types:

- **ValueWithFilter**: Contains `filterMask`, `ignoreMask`, and `value`. The `matches()` method checks `(otherValue & filterMask) == value` with fallback to ignore mask.
- **FilterList**: A main filter plus up to 4 ignorable filters.
- **NoteContainer**: Pre-filtered per-note-number sample lookup tables. Used for cached layers (like keyswitches) to avoid iterating all samples.

### Sound Collection Flow

1. `preHiseEventCallback(e)` -- all MIDI processors handle the event (keyswitches update filter, RR advances, etc.)
2. `collectSounds(m, soundsToBeStarted)`:
   - Iterate NoteContainers (groups), skip those not matching currentGroupFilter
   - For each sample at the note number, check `matchesCurrentFilter()` (bitmask + velocity)
   - After collection, run all postProcessors (RR advances counter, release trigger fires, legato processes, choke fades)

---

## Layer Type Implementations

### RRLayer (RoundRobin)

- Flags: `FlagProcessHiseEvent | FlagProcessPostSounds`
- On noteOn: applies current RR group as filter
- After voice start: increments RR counter, wraps around
- Reset: counter back to 1

### KeyswitchLayer

- Flags: `FlagProcessHiseEvent` (added automatically)
- Watches `SampleIds::LoKey` and `groupIds::isChromatic` for key range
- Builds a 128-entry `layerValues` array mapping note numbers to group values
- On noteOn: if note is a keyswitch note, applies that group as filter
- Reset: applies group 1 if not already initialized

### ReleaseTriggerLayer

- Flags: `FlagProcessHiseEvent | FlagProcessPostSounds`
- Fixed two groups: SustainGroupIndex=1, ReleaseGroupIndex=2
- On noteOn: applies sustain filter
- On noteOff: ignores the note-off event, switches to release filter, fires artificial noteOn
- After voice start (release): fades out sustain voices, optionally matches gain
- Properties: `matchGain`, `fadeTime`, `accuracy`
- When matchGain is enabled: sets `setIsReleaseSample(true)` on sustain samples for peak tracking

### XFadeLayer

- Flags: `FlagProcessModulation | FlagProcessHiseEvent` (added in onDataChange)
- Provides per-voice gain modulation via `calculateGroupModulationValuesForVoice()`
- Three input sources: EventData (AdditionalEventStorage), MidiCC, GlobalMod
- Five fader curves: Linear, RMS, CosineHalf, Overlap, Switch
- Supports up to 8 crossfade groups (hardcoded getFadeValue switch)
- Uses smoothed gains per voice with configurable fade time
- GlobalMod source connects to GlobalModulatorContainer (VoiceStart, TimeVariant, or Envelope)

### LegatoLayer

- Flags: `FlagProcessHiseEvent | FlagProcessPostSounds`
- Tokens are note numbers (128-entry array)
- Tracks last played note; on new note, sets filter to last note's layer value
- After voice start: calculates zero crossings for phase-locked transitions
- Computes delay and start offset to align transition samples at zero crossings
- TODO comments in source indicate this is still under development

### ChokeLayer

- Flags: `FlagProcessHiseEvent | FlagProcessPostSounds`
- Uses a matrix string encoding for choke group relationships
- On voice start: checks if started group's matrix value matches any active voice's group matrix value
- Matching voices get faded out with configurable fadeTime
- Matrix encoding: digits 0-9 and 'X' (IgnoreFlag)

### CustomLayer

- Flags: dynamically toggled via `groupStart` and `gainMod` properties
- Supports voice start callbacks (registered via scripting API)
- Provides per-group gain modulation with smoothed values
- `setGroupVolume()` sets per-group gain factors stored in `layerGains` vector
- Can be dynamically promoted to post-processor when a voice start callback is registered

---

## StartData System

Per-voice start modifications (delay, offset, fade-in, fixed-length) are stored in `activeDelayLayers`:

```cpp
struct StartData
{
    enum class Type
    {
        DelayTimeIndex,
        StartOffsetIndex,
        FadeTimeIndex,
        FadeOutOffset,
        TargetVolume,
        numStartData
    };
    uint8 layerIndex = 0;
    uint8 layerValue = 0;
    std::array<double, (int)Type::numStartData> data;
};
```

The `applyEventDataInternal()` method manages an `UnorderedStack<StartData>` that is queried by `getSpecialSoundStart()` during voice startup. Each entry is keyed by (layerIndex, layerValue) pair. The returned `SpecialStart` struct carries delay, offset, fade-in time, fixed length, and target volume to the voice startup code.

---

## Scripting API Method Implementation Details

### setActiveGroup(layerIndex, groupIndex)

Rejects IgnoreFlag with script error. Bumps to one-based. Calls `gm->applyFilter(layerIndex, gi, sendNotificationAsync)`. This sets the layer's filter value, which determines which samples will be started on the next noteOn.

### setLayerProperty / getLayerProperty

Uses `getManager()->getDataTree().getChild(idx)` to access the ValueTree for the layer. Validates property ID via `Helpers::isValidId()`. Uses `Helpers::convertFromJS()` / `convertToJS()` for type conversion (handles `matrixString` as int array, `tokens` as string array).

### createNoteMap(layerIdOrIndex)

Iterates all samples, checks each sample's layer value. If the sample has a non-zero, non-IgnoreFlag value for this layer, marks its note range in a `VoiceBitMap<128, uint32>`. Stores the map in `noteMaps` keyed by layer index. Must be called before `isNoteNumberMapped()`.

### isNoteNumberMapped(layerIndex, noteNumber)

Looks up `noteMaps` by layer index. Reports script error if `createNoteMap()` wasn't called first. Returns whether the note number has any mapped samples.

### setEnableGainTracking(layerIdOrIndex, groupIndex, shouldBeActive)

Rejects IgnoreFlag. Records the (layerIndex, groupIndex) pair in `gainTrackingGroups`. Iterates all samples matching the layer/group and calls `setIsReleaseSample(shouldBeActive)` on each -- this repurposes the release sample peak tracking mechanism for gain tracking.

### getCurrentPeak(layerIndex, groupIndex, eventId)

Validates that gain tracking was enabled for this layer/group pair. Iterates active voices, finds the voice matching the eventId and layer/group, returns `getCurrentReleasePeak()` from its sound reference.

### registerGroupStartCallback(layerIdOrIndex, callback)

Requires a realtime-safe callable object (checked via `RealtimeSafetyInfo::check` in backend, `isRealtimeSafe()` in frontend). Creates a `GroupCallback` wrapper and registers it via `gm->setVoiceStartCallback()`. The callback receives the zero-based group index (or IgnoreFlag) as its single argument.

When registered on a CustomLayer, the layer is automatically promoted to a post-processor to ensure `postVoiceStart()` fires.

### delayGroupEvent / fadeInGroupEvent / setFixedGroupEventLength / addGroupEventStartOffset

All follow the same pattern: bump groupIndex to one-based, call the corresponding `applyEventDataInternal()` via the named wrapper methods on ComplexGroupManager.

Note: `fadeInGroupEvent` converts ms to seconds (`fadeInTimeMs * 0.001`) and dB to gain factor (`Decibels::decibelsToGain(targetGainDb)`) before storing.

### fadeOutGroupEvent(layerIndex, groupIndex, fadeOutTimeMs)

Different from the others -- it directly iterates `sampler->activeVoices`, finds voices whose sound's bitmask layer value matches, and calls `av->setVolumeFade(fadeOutTimeMs * 0.001, 0.0)` to fade them to silence. This operates on already-playing voices rather than setting up start data for future voices.

### setGroupVolume(layerIndex, groupIndex, gainFactor)

Bumps to one-based, calls `gm->setGroupVolume()` which casts to `CustomLayer*` and sets `layerGains[value-1]`. Only works with Custom logic type layers.

---

## Threading Considerations

- **Audio thread methods**: `setActiveGroup`, `delayGroupEvent`, `fadeInGroupEvent`, `setFixedGroupEventLength`, `addGroupEventStartOffset`, `fadeOutGroupEvent`, `setGroupVolume`, `getCurrentPeak`, `isNoteNumberMapped` -- these are called during note processing
- **Init-time methods**: `createNoteMap`, `setEnableGainTracking`, `registerGroupStartCallback`, `setLayerProperty`, `getLayerProperty` -- these set up state
- The `registerGroupStartCallback` explicitly checks for realtime safety of the callback
- `fadeOutGroupEvent` iterates `sampler->activeVoices` which is an audio-thread data structure
- The underlying `applyFilter()` is called from both MIDI processors (audio thread) and scripting (message thread), using `sendNotificationAsync` for thread-safe state updates

---

## Preprocessor Guards

- `USE_BACKEND`: Used in `registerGroupStartCallback` for realtime safety checking (`RealtimeSafetyInfo::check`), in XFadeLayer for `valueUpdater` UI updates, and in `setSampler()` for `registerComplexGroupManager()`
- `HI_ENABLE_EXPANSION_EDITING`: Used alongside `USE_BACKEND` in `setSampler()` for sample edit handler registration
- `JUCE_DEBUG`: `dumpLayers()` method for debug output

---

## Data Tree Structure

The ComplexGroupManager stores its configuration in a `ValueTree` with structure:

```
Layers (root)
  Layer (child 0)
    type: "RR"
    id: "RR"
    tokens: "RR1,RR2,RR3"
    ignorable: false
    cached: false
    purgable: false
    ...
  Layer (child 1)
    type: "Keyswitch"
    id: "Articulation"
    tokens: "sustain,staccato,legato"
    ...
```

Changes to this tree trigger `onDataChange()` which rebuilds the layer objects, and `onRebuildPropertyChange()` which triggers a full rebuild when tokens/flags change.
