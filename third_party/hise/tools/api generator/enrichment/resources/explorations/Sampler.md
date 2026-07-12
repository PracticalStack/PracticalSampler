# Sampler -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` (Sampler entry)
- `enrichment/base/Sampler.json` (56 methods)
- No prerequisites needed (Sampler is itself a prerequisite for Sample and ComplexGroupManager)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.h`, line 881

```cpp
class Sampler : public ConstScriptingObject
{
public:
    Sampler(ProcessorWithScriptingContent *p, ModulatorSampler *sampler);
    ~Sampler() {};

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("Sampler"); }
    Identifier getObjectName() const override { return getClassName(); }
    bool objectDeleted() const override { return sampler.get() == nullptr; }
    bool objectExists() const override { return sampler.get() != nullptr; }

    // ... API methods ...

    struct Wrapper;

private:
    ValueTree convertJSONListToValueTree(var jsonSampleList);
    WeakReference<Processor> sampler;
    SelectedItemSet<ModulatorSamplerSound::Ptr> soundSelection;
    Array<Identifier> sampleIds;
};
```

**Key observations:**
- Extends `ConstScriptingObject` -- a handle class, not a namespace/ApiClass
- Holds a `WeakReference<Processor>` to the underlying `ModulatorSampler`
- Has its own `SelectedItemSet<ModulatorSamplerSound::Ptr> soundSelection` for the legacy selectSounds/getNumSelectedSounds/setSoundProperty workflow
- Stores `Array<Identifier> sampleIds` -- the SampleIds namespace identifiers, used for property indexing
- The `convertJSONListToValueTree` private helper converts JSON sample arrays to ValueTree format for samplemap loading

## Underlying Processor: ModulatorSampler

**File:** `HISE/hi_core/hi_sampler/sampler/ModulatorSampler.h`, line 55

```cpp
class ModulatorSampler: public ModulatorSynth,
                        public LookupTableProcessor,
                        public SuspendableTimer::Manager,
                        public NonRealtimeProcessor
```

`SET_PROCESSOR_NAME("StreamingSampler", "Sampler", "The main sampler class of HISE.");`

### ModulatorSampler Parameters (Attributes)

The attribute system exposes `ModulatorSampler::Parameters` enum, which extends `ModulatorSynth::numModulatorSynthParameters`:

```cpp
enum Parameters
{
    PreloadSize = ModulatorSynth::numModulatorSynthParameters,
    BufferSize,
    VoiceAmount,
    RRGroupAmount,
    SamplerRepeatMode,
    PitchTracking,
    OneShot,
    CrossfadeGroups,
    Purged,
    Reversed,
    UseStaticMatrix,
    LowPassEnvelopeOrder,
    numModulatorSamplerParameters
};
```

These are accessed via `getAttribute(index)` / `setAttribute(index, value)`. The index includes the base class parameters from `ModulatorSynth` (inherited from `Processor`).

### RepeatMode enum

```cpp
enum RepeatMode
{
    KillNote = 0,        // kills the note (using the supplied fade time)
    NoteOff,             // triggers a note off event before starting the note
    DoNothing,           // a new voice is started and the old keeps ringing
    KillSecondOldestNote,// allow one note to retrigger, then kill
    KillThirdOldestNote
};
```

### Internal Chains

```cpp
enum InternalChains
{
    SampleStartModulation = ModulatorSynth::numInternalChains,
    CrossFadeModulation,
    numInternalChains
};
```

## Factory / obtainedVia

Sampler objects are created in three ways:

1. **Automatic registration in script processors** (`ScriptProcessorModules.cpp:304`):
   ```cpp
   samplerObject = new ScriptingApi::Sampler(this, dynamic_cast<ModulatorSampler*>(getOwnerSynth()));
   ```
   This creates the `Sampler` object automatically when a script processor is inside a ModulatorSampler module. If the owner synth is not a ModulatorSampler, the internal pointer is null and all methods report errors.

2. **Via `ChildSynth.asSampler()`** (`ScriptingApiObjects.cpp:4517`):
   ```cpp
   var ScriptingObjects::ScriptingSynth::asSampler()
   {
       if (checkValidObject())
       {
           auto sampler = dynamic_cast<ModulatorSampler*>(synth.get());
           if (sampler == nullptr)
               return var(); // don't complain here, handle it on scripting level
           auto t = new ScriptingApi::Sampler(getScriptProcessor(), sampler);
           return var(t);
       }
       auto t = new ScriptingApi::Sampler(getScriptProcessor(), nullptr);
       return var(t);
   }
   ```
   Returns undefined if the ChildSynth is not a sampler (no error thrown).

3. **Via `Synth.asSampler()`** -- same pattern as ChildSynth.

The `Sampler` object is available as a global API class in script processors. When the script processor lives inside a Sampler module, `Sampler` is automatically available. For external samplers, use `ChildSynth.asSampler()`.

## Constructor -- Constants Registration

**File:** `ScriptingApi.cpp`, line 3839

```cpp
ScriptingApi::Sampler::Sampler(ProcessorWithScriptingContent *p, ModulatorSampler *sampler_) :
    ConstScriptingObject(p, SampleIds::numProperties),  // numProperties = 25
    sampler(sampler_)
{
    // ... ADD_API_METHOD registrations ...

    sampleIds = SampleIds::Helpers::getAllIds();

    for (int i = 1; i < sampleIds.size(); i++)
    {
        addConstant(sampleIds[i].toString(), (int)i);
    }
}
```

The constant count (`SampleIds::numProperties = 25`) is passed to `ConstScriptingObject`. Constants are registered from index 1 (skipping `Unused` at index 0).

## SampleIds Constants (Sound Property Constants)

**File:** `HISE/hi_core/hi_sampler/sampler/ModulatorSamplerSound.h`, line 118

```cpp
namespace SampleIds
{
    DECLARE_ID(Unused);           // 0 -- skipped
    DECLARE_ID(ID);               // 1
    DECLARE_ID(FileName);         // 2
    DECLARE_ID(Root);             // 3
    DECLARE_ID(HiKey);            // 4
    DECLARE_ID(LoKey);            // 5
    DECLARE_ID(LoVel);            // 6
    DECLARE_ID(HiVel);            // 7
    DECLARE_ID(RRGroup);          // 8
    DECLARE_ID(Volume);           // 9
    DECLARE_ID(Pan);              // 10
    DECLARE_ID(Normalized);       // 11
    DECLARE_ID(NormalizedPeak);   // 12
    DECLARE_ID(Pitch);            // 13
    DECLARE_ID(SampleStart);      // 14
    DECLARE_ID(SampleEnd);        // 15
    DECLARE_ID(SampleStartMod);   // 16
    DECLARE_ID(LoopStart);        // 17
    DECLARE_ID(LoopEnd);          // 18
    DECLARE_ID(LoopXFade);        // 19
    DECLARE_ID(LoopEnabled);      // 20
    DECLARE_ID(ReleaseStart);     // 21
    DECLARE_ID(LowerVelocityXFade); // 22
    DECLARE_ID(UpperVelocityXFade); // 23
    DECLARE_ID(SampleState);      // 24
    DECLARE_ID(Reversed);         // 25 -- but wait...
    DECLARE_ID(GainTable);
    DECLARE_ID(PitchTable);
    DECLARE_ID(LowPassTable);
    DECLARE_ID(NumQuarters);

    const int numProperties = 25;
}
```

Note: `numProperties = 25` is hardcoded. The constants registered are indices 1-24 (ID through SampleState). The identifiers after index 24 (Reversed, GainTable, PitchTable, LowPassTable, NumQuarters) exist in the namespace but are beyond numProperties, so they are NOT registered as script constants. However, the `sampleIds` array from `getAllIds()` may include them -- need to verify. The constructor loops `for (int i = 1; i < sampleIds.size(); i++)` so the actual number of constants depends on what `getAllIds()` returns.

Looking at the `ModulatorSamplerSound::Property` enum (same file, line 193):
```cpp
enum Property
{
    ID = 1,
    FileName,        // 2
    RootNote,        // 3
    HiKey,           // 4
    LoKey,           // 5
    LoVel,           // 6
    HiVel,           // 7
    RRGroup,         // 8
    Volume,          // 9
    Pan,             // 10
    Normalized,      // 11
    Pitch,           // 12 -- note: NormalizedPeak is skipped here
    SampleStart,     // 13
    SampleEnd,       // 14
    SampleStartMod,  // 15
    LoopStart,       // 16
    LoopEnd,         // 17
    LoopXFade,       // 18
    LoopEnabled,     // 19
    ReleaseStart,    // 20
    LowerVelocityXFade, // 21
    UpperVelocityXFade, // 22
    SampleState,     // 23
    Reversed,        // 24
    GainTable,       // 25
    PitchTable,      // 26
    numProperties    // 27
};
```

Important: The `ModulatorSamplerSound::Property` enum and `SampleIds` namespace have a slight mismatch in ordering (NormalizedPeak exists in SampleIds but not in the Property enum). The scripting constants use the SampleIds indices, NOT the enum values.

## Wrapper Struct -- Method Registration Types

**File:** `ScriptingApi.cpp`, line 3778

All methods use standard `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` macros -- no typed wrappers. The constructor uses both `ADD_API_METHOD_N` (untyped) and `ADD_TYPED_API_METHOD_N` (typed) registrations.

### Typed API Methods (ADD_TYPED_API_METHOD)

```cpp
ADD_TYPED_API_METHOD_1(getAttribute, VarTypeChecker::Number);
ADD_TYPED_API_METHOD_1(getAttributeId, VarTypeChecker::Number);
ADD_TYPED_API_METHOD_1(getAttributeIndex, VarTypeChecker::String);
ADD_TYPED_API_METHOD_2(setAttribute, VarTypeChecker::Number, VarTypeChecker::Number);
ADD_TYPED_API_METHOD_1(createSelectionFromIndexes, VarTypeChecker::Array);
```

All other methods use untyped `ADD_API_METHOD_N`.

## Threading Patterns

### WARN_IF_AUDIO_THREAD Methods
Many methods use `WARN_IF_AUDIO_THREAD(true, ScriptGuard::IllegalApiCall)`:
- `enableRoundRobin`, `refreshRRMap`
- `createSelection`, `createSelectionFromIndexes`
- `selectSounds`, `getNumSelectedSounds`
- `setSoundPropertyForSelection`, `setSoundPropertyForAllSamples`
- `getSoundProperty`, `setSoundProperty`
- `purgeMicPosition`, `getMicPositionName`
- `refreshInterface`
- `loadSampleMap`, `loadSfzFile`
- `loadSampleForAnalysis`
- `getCurrentSampleMapId`, `getSampleMapList`
- `setUseStaticMatrix`, `setSortByRRGroup`
- `setGUISelection`
- `importSamples`

### Audio-Thread Callable Methods
These do NOT have WARN_IF_AUDIO_THREAD:
- `setActiveGroup` (delegates to `setActiveGroupForEventId`)
- `setActiveGroupForEventId` -- has explicit audio thread check for eventId != -1
- `setMultiGroupIndex`, `setMultiGroupIndexForEventId`
- `setRRGroupVolume`
- `setAllowReleaseStart`
- `getActiveRRGroup`, `getActiveRRGroupForEventId`
- `getNumActiveGroups`
- `getRRGroupsForMessage`
- `isNoteNumberMapped`
- `getAttribute`, `setAttribute`
- `getNumAttributes`, `getAttributeId`, `getAttributeIndex`

### killAllVoicesAndCall Pattern
Several methods use voice-killing for safe async operations:
- `loadSampleMap` -- `s->killAllVoicesAndCall(..., true)`
- `loadSampleMapFromJSON` -- `s->killAllVoicesAndCall(..., true)`
- `loadSampleMapFromBase64` -- `s->killAllVoicesAndCall(..., true)`
- `purgeSampleSelection` -- `s->killAllVoicesAndCall(f, true)`
- `clearSampleMap` -- `s->killAllVoicesAndCall(f)`
- `loadSfzFile` -- `s->killAllVoicesAndCall(...)`

### callAsyncIfJobsPending Pattern
Used for property modifications:
- `setSoundPropertyForSelection` -- `s->callAsyncIfJobsPending(f)`
- `setSoundPropertyForAllSamples` -- `s->callAsyncIfJobsPending(f)`
- `purgeMicPosition` -- `s->callAsyncIfJobsPending(f)`

### Event-ID Thread Restriction
`setActiveGroupForEventId` has an explicit check:
```cpp
if(eventId != -1 && s->getMainController()->getKillStateHandler().getCurrentThread()
    != MainController::KillStateHandler::TargetThread::AudioThread)
{
    reportScriptError("This method is only available in the onNoteOnCallback");
    return;
}
```
This means per-event group setting only works on the audio thread (note-on callback).

## Sample Selection Systems

There are TWO distinct selection systems:

### 1. Legacy Script Selection (soundSelection member)
- `selectSounds(regex)` -- fills `soundSelection` using `ModulatorSamplerSound::selectSoundsBasedOnRegex`
- `getNumSelectedSounds()` -- returns `soundSelection.getNumSelected()`
- `getSoundProperty(propertyIndex, soundIndex)` -- reads from `soundSelection.getSelectedItem(soundIndex)`
- `setSoundProperty(soundIndex, propertyIndex, newValue)` -- writes to `soundSelection.getSelectedItem(soundIndex)`
- `setSoundPropertyForSelection(propertyIndex, newValue)` -- applies to all items in `soundSelection`
- `loadSampleForAnalysis(indexInSelection)` -- reads from `soundSelection`
- `createListFromScriptSelection()` -- converts `soundSelection` items to `ScriptingSamplerSound` objects

### 2. Modern Selection (returns Sample/ScriptingSamplerSound arrays)
- `createSelection(regex)` -- creates new `ScriptingSamplerSound` objects matching regex
- `createSelectionFromIndexes(indexData)` -- creates from index/array/-1
- `createSelectionWithFilter(filterFunction)` -- creates via callback filter
- `createListFromGUISelection()` -- **USE_BACKEND only**, reads from sample edit handler

The modern API returns arrays of `ScriptingSamplerSound` objects (the `Sample` class in script). The legacy API uses an internal `SelectedItemSet` and integer indices.

## Preprocessor Guards

### USE_BACKEND
- `createListFromGUISelection()` -- entire implementation behind `#if USE_BACKEND`, returns `{}` in frontend
- `setGUISelection()` -- implementation behind `#if USE_BACKEND`

### HI_ENABLE_EXPANSION_EDITING
- `importSamples()` -- entire implementation behind `#if HI_ENABLE_EXPANSION_EDITING`, returns `{}` otherwise
- Default: enabled in backend (`1`), disabled in frontend (`0`)

### HISE_SAMPLER_ALLOW_RELEASE_START
- `getReleaseStartOptions()` -- reports error if not enabled
- `setReleaseStartOptions()` -- reports error if not enabled
- `setAllowReleaseStart()` -- delegates to `ModulatorSampler::setAllowReleaseStart()` (presumably also guarded)
- Default: `1` (enabled by default, per `hi_streaming.h:96`)

## TimestretchOptions JSON Schema

**File:** `ModulatorSampler.h`, line 263

```cpp
struct TimestretchOptions: public RestorableObject
{
    enum class TimestretchMode
    {
        Disabled,     // no timestretching
        VoiceStart,   // new voices use current ratio, active keep theirs
        TimeVariant,  // all active voices use the same ratio
        TempoSynced,  // ratio based on sample length and current tempo
        numTimestretchModes
    };

    TimestretchMode mode = TimestretchMode::Disabled;
    double tonality = 0.0;
    bool synchronousSkip = false;
    double numQuarters = 0.0;
    Identifier engineId;
};
```

JSON properties (from `toJSON()`):
- `"Mode"`: String -- `"Disabled"`, `"VoiceStart"`, `"TimeVariant"`, `"TempoSynced"`
- `"Tonality"`: double -- 0.0 to 1.0
- `"SkipLatency"`: bool
- `"NumQuarters"`: double
- `"PreferredEngine"`: String (engine identifier)

## ReleaseStartOptions JSON Schema

**File:** `hi_streaming/StreamingSampler.h`, line 77

```cpp
struct ReleaseStartOptions: public ReferenceCountedObject
{
    enum class GainMatchingMode
    {
        None,
        Volume,
        Offset,
        numGainMatchingModes
    };

    int releaseFadeTime = 4096;
    float fadeGamma = 1.0;
    bool useAscendingZeroCrossing = false;
    GainMatchingMode gainMatchingMode = GainMatchingMode::None;
    float smoothing = 0.96f;
};
```

JSON properties (from `toJSON()`):
- `"ReleaseFadeTime"`: int -- 0 to 44100, default 4096
- `"FadeGamma"`: float -- 0.125 to 4.0, default 1.0 (clamped to 0.0-2.0 in fromJSON)
- `"UseAscendingZeroCrossing"`: bool -- default false
- `"GainMatchingMode"`: String -- `"None"`, `"Volume"`, `"Offset"`
- `"PeakSmoothing"`: float -- default 0.96

## convertJSONListToValueTree Helper

**File:** `ScriptingApi.cpp`, line 5240

This private method converts a JSON array of sample objects to a ValueTree for samplemap loading:

```cpp
ValueTree ScriptingApi::Sampler::convertJSONListToValueTree(var jsonSampleList)
{
    if (auto a = jsonSampleList.getArray())
    {
        auto v = ValueTreeConverters::convertVarArrayToFlatValueTree(jsonSampleList, "samplemap", "sample");
        v.setProperty("ID", "CustomJSON", nullptr);
        v.setProperty("SaveMode", 0, nullptr);
        v.setProperty("RRGroupAmount", 1, nullptr);
        v.setProperty("MicPositions", ";", nullptr);

        // Add missing defaults per sample
        for (auto c : v)
        {
            addMissingProp(c, SampleIds::LoVel, 0);
            addMissingProp(c, SampleIds::HiVel, 127);
            addMissingProp(c, SampleIds::LoKey, 0);
            addMissingProp(c, SampleIds::HiKey, 127);
            addMissingProp(c, SampleIds::Root, 64);
            addMissingProp(c, SampleIds::RRGroup, 1);
        }
        return v;
    }
    return {};
}
```

Default values applied when loading from JSON:
- LoVel: 0, HiVel: 127 (full velocity range)
- LoKey: 0, HiKey: 127 (full key range)
- Root: 64 (middle C-ish)
- RRGroup: 1

## Created Objects

### ScriptingSamplerSound (Sample)
Many methods return arrays of `ScriptingSamplerSound` objects:
- `createSelection()`, `createSelectionFromIndexes()`, `createSelectionWithFilter()`
- `createListFromScriptSelection()`, `createListFromGUISelection()`
- `importSamples()`

Each is created as: `new ScriptingObjects::ScriptingSamplerSound(getScriptProcessor(), s, sound)`

### ScriptingComplexGroupManager (ComplexGroupManager)
- `getComplexGroupManager()` returns `new ScriptingObjects::ScriptingComplexGroupManager(getScriptProcessor(), sampler)`

## Round Robin Group Management

The Sampler class provides extensive RR group control:

1. **Enable/disable automatic RR** -- `enableRoundRobin(bool)` calls `s->setUseRoundRobinLogic()`
2. **Single-group activation** -- `setActiveGroup(index)` (delegates to eventId=-1 version)
3. **Per-event group** -- `setActiveGroupForEventId(eventId, index)` -- audio thread only for eventId != -1
4. **Multi-group activation** -- `setMultiGroupIndex(groupIndex, enabled)` supports:
   - Single int group index
   - Array of group indices
   - MidiList object (bulk set via raw pointer)
5. **Per-event multi-group** -- `setMultiGroupIndexForEventId(eventId, groupIndex, enabled)` -- same flexibility
6. **Group volume** -- `setRRGroupVolume(groupIndex, gainInDecibels)` -- -1 for active group
7. **RR map queries** -- `getRRGroupsForMessage(noteNumber, velocity)` requires `refreshRRMap()` first
8. **Sort optimization** -- `setSortByRRGroup(bool)` for large sample sets (>20,000 samples)

Key constraint: `setActiveGroup`, `setMultiGroupIndex`, `getRRGroupsForMessage` all require `enableRoundRobin(false)` first. If RR is enabled, they report script errors.

## Mic Position Management

- `getNumMicPositions()` -- returns channel count
- `getMicPositionName(channelIndex)` -- returns `s->getChannelData(channelIndex).suffix`
- `isMicPositionPurged(micIndex)` -- returns `!s->getChannelData(micIndex).enabled`
- `purgeMicPosition(micName, shouldBePurged)` -- matches by name string, calls `s->setMicEnabled(i, !shouldBePurged)` via async
- `purgeSampleSelection(selection)` -- purges specific Sample objects, unpurges the rest

Both `getMicPositionName` and `purgeMicPosition` check for multi-mic mode: `!s->isUsingStaticMatrix() && s->getNumMicPositions() == 1` reports error.

## Sample Map Loading/Saving

Multiple loading paths:
1. `loadSampleMap(fileName)` -- by pool reference string, uses `killAllVoicesAndCall`
2. `loadSampleMapFromJSON(jsonSampleMap)` -- via `convertJSONListToValueTree`, uses `killAllVoicesAndCall`
3. `loadSampleMapFromBase64(b64)` -- zstd-compressed ValueTree, uses `killAllVoicesAndCall`
4. `loadSfzFile(sfzFile)` -- accepts `ScriptFile` or absolute path string, uses `SfzImporter`

Export/save:
1. `getSampleMapAsBase64()` -- zstd compress + base64 encode
2. `saveCurrentSampleMap(relativePath)` -- saves to SampleMaps directory as XML
3. `clearSampleMap()` -- calls `sm->clear(sendNotificationAsync)` via `killAllVoicesAndCall`

Query:
1. `getCurrentSampleMapId()` -- returns map ID string
2. `getSampleMapList()` -- returns sorted array of all available samplemap references

## importSamples Method

**Guarded by:** `#if HI_ENABLE_EXPANSION_EDITING` (backend/expansion editing only)

This method:
1. Kills voices and waits synchronously
2. Acquires SampleLock
3. Optionally skips existing samples (by filename comparison)
4. Uses `SampleImporter::loadAudioFilesUsingDropPoint()` with auto root note assignment
5. Returns array of `ScriptingSamplerSound` objects for newly imported samples
6. Extends script timeout for the import duration

## parseSampleFile Method

Accepts `ScriptFile` object or absolute path string. Calls `s->parseMetadata(f)` which returns a ValueTree, then converts to a DynamicObject. This produces a JSON object compatible with `loadSampleMapFromJSON`.

## getAudioWaveformContentAsBase64 Method

Specialized converter for user preset data from AudioWaveform components. Reads `data`, `rangeStart`, `rangeEnd` properties from the preset object, uses `parseSampleFile` internally, then converts to base64-compressed samplemap format.

## createSelectionWithFilter Callback

Uses `engine->callExternalFunctionRaw(filterFunction, args)` to evaluate the filter. The callback receives a `ScriptingSamplerSound` as `this`, takes no arguments, and should return non-zero to include the sample.

## GroupedRoundRobinCollector

**File:** `ModulatorSampler.h`, line 86

Inner class of ModulatorSampler that pre-sorts sounds into RR groups for efficient voice start:

```cpp
class GroupedRoundRobinCollector : public ModulatorSynth::SoundCollectorBase,
                                   public SampleMap::Listener,
                                   public AsyncUpdater
```

This is what `setSortByRRGroup(true)` activates. It listens to sample map changes and rebuilds the group structure asynchronously. Used for performance optimization with large sample counts.
