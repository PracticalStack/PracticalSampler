# Sample (ScriptingSamplerSound) -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` -- Sample entry
- `enrichment/phase1/Sampler/Readme.md` -- prerequisite class context
- `enrichment/base/Sample.json` -- base method list

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h:1402-1484`

```cpp
class ScriptingSamplerSound : public ConstScriptingObject,
                              public AssignableObject
{
public:
    ScriptingSamplerSound(ProcessorWithScriptingContent* p, ModulatorSampler* ownerSampler, ModulatorSamplerSound::Ptr sound_);
    ~ScriptingSamplerSound() {};

    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Sample"); }
    String getDebugName() const override { return "Sample"; };
    String getDebugValue() const override;

    int getNumChildElements() const override { return (int)sampleIds.size() + (int)customObject.isObject(); }
    DebugInformation* getChildElement(int index) override;

    bool objectDeleted() const override { return sound == nullptr; }
    bool objectExists() const override { return sound != nullptr; }

    // AssignableObject interface
    void assign(const int index, var newValue) override;
    var getAssignedValue(int index) const override;
    int getCachedIndex(const var &indexExpression) const override;

    ModulatorSamplerSound::Ptr getSoundPtr() { return sound; }

private:
    var customObject;
    ModulatorSampler* getSampler() const;
    Array<Identifier> sampleIds;
    struct Wrapper;
    WeakReference<Processor> sampler;
    ModulatorSamplerSound::Ptr sound;
};
```

### Inheritance

1. **ConstScriptingObject** -- standard HISE scripting API base class. Provides `addConstant()`, `ADD_API_METHOD_N` registration, `reportScriptError()`, `getScriptProcessor()`.
2. **AssignableObject** -- enables bracket-based property access syntax. Three pure virtual methods:
   - `assign(index, newValue)` -- delegates to `set(index, newValue)`
   - `getAssignedValue(index)` -- delegates to `get(index)`
   - `getCachedIndex(indexExpression)` -- resolves string identifiers to integer indices via `sampleIds.indexOf()`

The AssignableObject interface means scripts can use:
- `sample[Sample.Root]` (integer constant index)
- `sample["Root"]` (string identifier, resolved via getCachedIndex)

### Object Lifetime

The `sound` member is a `ModulatorSamplerSound::Ptr` (ReferenceCountedObjectPtr). If the underlying sound is removed from the sampler, the reference becomes null. All API methods check `objectExists()` before proceeding and report a script error if the sound is gone. The `sampler` member is a `WeakReference<Processor>` -- `getSampler()` checks for null and reports error if the sampler processor was deleted.

## Constructor Analysis

**File:** `ScriptingApiObjects.cpp:2432-2478`

```cpp
ScriptingSamplerSound(ProcessorWithScriptingContent* p, ModulatorSampler* sampler_, ModulatorSamplerSound::Ptr sound_) :
    ConstScriptingObject(p, ModulatorSamplerSound::numProperties),
    sound(sound_),
    sampler(sampler_)
```

The second argument to `ConstScriptingObject` is `ModulatorSamplerSound::numProperties` (= 25 from enum), which sets the number of constant slots.

### sampleIds Array Population

The constructor builds an ordered array of `Identifier` objects that maps integer indices to SampleIds:

| Index | SampleId | Constant Name | Constant Value |
|-------|----------|---------------|----------------|
| 0 | SampleIds::ID | (not exposed) | -- |
| 1 | SampleIds::FileName | FileName | 1 |
| 2 | SampleIds::Root | Root | 2 |
| 3 | SampleIds::HiKey | HiKey | 3 |
| 4 | SampleIds::LoKey | LoKey | 4 |
| 5 | SampleIds::LoVel | LoVel | 5 |
| 6 | SampleIds::HiVel | HiVel | 6 |
| 7 | SampleIds::RRGroup | RRGroup | 7 |
| 8 | SampleIds::Volume | Volume | 8 |
| 9 | SampleIds::Pan | Pan | 9 |
| 10 | SampleIds::Normalized | Normalized | 10 |
| 11 | SampleIds::Pitch | Pitch | 11 |
| 12 | SampleIds::SampleStart | SampleStart | 12 |
| 13 | SampleIds::SampleEnd | SampleEnd | 13 |
| 14 | SampleIds::SampleStartMod | SampleStartMod | 14 |
| 15 | SampleIds::LoopStart | LoopStart | 15 |
| 16 | SampleIds::LoopEnd | LoopEnd | 16 |
| 17 | SampleIds::LoopXFade | LoopXFade | 17 |
| 18 | SampleIds::LoopEnabled | LoopEnabled | 18 |
| 19 | SampleIds::ReleaseStart | ReleaseStart | 19 |
| 20 | SampleIds::LowerVelocityXFade | LowerVelocityXFade | 20 |
| 21 | SampleIds::UpperVelocityXFade | UpperVelocityXFade | 21 |
| 22 | SampleIds::SampleState | SampleState | 22 |
| 23 | SampleIds::Reversed | Reversed | 23 |

**Note:** Index 0 (SampleIds::ID) is NOT exposed as a constant -- the `addConstant` loop starts from `i = 1`. Constants are added via:
```cpp
for (int i = 1; i < sampleIds.size(); i++)
    addConstant(sampleIds[i].toString(), (int)i);
```

### Method Registrations

All methods use plain `ADD_API_METHOD_N` (no typed variants):

```cpp
ADD_API_METHOD_1(setFromJSON);
ADD_API_METHOD_1(get);
ADD_API_METHOD_2(set);
ADD_API_METHOD_1(getRange);
ADD_API_METHOD_0(deleteSample);
ADD_API_METHOD_0(duplicateSample);
ADD_API_METHOD_0(loadIntoBufferArray);
ADD_API_METHOD_1(replaceAudioFile);
ADD_API_METHOD_1(refersToSameSample);
ADD_API_METHOD_0(getSampleRate);
ADD_API_METHOD_0(getCustomProperties);
```

**Note:** `getId` is declared in the header but NOT registered via ADD_API_METHOD. It IS in the base JSON though. Looking at the Wrapper struct, there is no wrapper for `getId` either. This method appears in the header's public section but is not registered as a scripting API method. The base JSON includes it, so it may be registered through a different mechanism or may be an artifact.

Actually, re-checking: `getId` IS in the base JSON (from Doxygen) but is NOT in the Wrapper struct and NOT in ADD_API_METHOD calls. This is unusual -- it suggests either a registration mechanism I missed, or it's exposed through the constant system rather than as a callable method.

## Wrapper Struct

**File:** `ScriptingApiObjects.cpp:2417-2430`

All wrappers use standard `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` macros (no typed variants):

```cpp
struct ScriptingObjects::ScriptingSamplerSound::Wrapper
{
    API_VOID_METHOD_WRAPPER_1(ScriptingSamplerSound, setFromJSON);
    API_METHOD_WRAPPER_1(ScriptingSamplerSound, get);
    API_VOID_METHOD_WRAPPER_2(ScriptingSamplerSound, set);
    API_VOID_METHOD_WRAPPER_0(ScriptingSamplerSound, deleteSample);
    API_METHOD_WRAPPER_0(ScriptingSamplerSound, duplicateSample);
    API_METHOD_WRAPPER_0(ScriptingSamplerSound, loadIntoBufferArray);
    API_METHOD_WRAPPER_1(ScriptingSamplerSound, replaceAudioFile);
    API_METHOD_WRAPPER_0(ScriptingSamplerSound, getSampleRate);
    API_METHOD_WRAPPER_1(ScriptingSamplerSound, getRange);
    API_METHOD_WRAPPER_1(ScriptingSamplerSound, refersToSameSample);
    API_METHOD_WRAPPER_0(ScriptingSamplerSound, getCustomProperties);
};
```

No wrapper for `getId` -- confirming it is not registered as a standard API method.

## Property System -- Delegation to ModulatorSamplerSound

The Sample class is a thin wrapper. Most methods delegate directly to `ModulatorSamplerSound`:

### get(propertyIndex)
```cpp
auto id = sampleIds[propertyIndex];
auto v = sound->getSampleProperty(id);
if (id == SampleIds::FileName)
    return v;  // returns string
else
    return var((int)v);  // returns int for all other properties
```

**Key behavior:** All non-FileName properties are cast to int on return, even if the underlying value is stored differently.

### set(propertyIndex, newValue)
```cpp
sound->setSampleProperty(sampleIds[propertyIndex], newValue);
```

Delegates directly. `setSampleProperty` in ModulatorSamplerSound:
- For GainTable/PitchTable/LowPassTable: stores as string (not exposed in Sample's sampleIds)
- For RRGroup: stores as int64 (bitmask)
- For all others: clips value to `getPropertyRange()` and stores as int

### setFromJSON(object)
Iterates over DynamicObject properties, calling `setSampleProperty(prop.name, prop.value)` for each. Uses Identifier-based property names (e.g., "Root", "HiKey"), not integer indices.

### getRange(propertyIndex)
Returns `[start, end]` array from `sound->getPropertyRange()`. Ranges are dynamic -- they depend on current property values (e.g., HiKey range starts at current LoKey value).

### Property Range Reference (from ModulatorSamplerSound::getPropertyRange)

| Property | Range |
|----------|-------|
| ID | 0 -- INT_MAX |
| FileName | empty range |
| Root | 0 -- 127 |
| HiKey | LoKey -- 127 |
| LoKey | 0 -- HiKey |
| LoVel | 0 -- (HiVel - LowerVelocityXFade - UpperVelocityXFade) |
| HiVel | (LoVel + LowerVelocityXFade + UpperVelocityXFade) -- 127 |
| Volume | -100 -- 18 |
| Pan | -100 -- 100 |
| Normalized | 0 -- 1 |
| RRGroup | 1 -- 255 |
| Pitch | -100 -- 100 |
| SampleStart | 0 -- (SampleEnd or LoopStart) |
| SampleEnd | (SampleStart + SampleStartMod) -- file length |
| SampleStartMod | 0 -- sample length |
| LoopStart | (SampleStart + LoopXFade) -- (LoopEnd - LoopXFade) |
| LoopEnd | (LoopStart + LoopXFade) -- SampleEnd |
| LoopXFade | 0 -- min(LoopStart - SampleStart, LoopLength) |
| LoopEnabled | 0 -- 1 |
| Reversed | 0 -- 1 |
| SampleState | 0 -- 2 (Normal/Disabled/Purged) |
| ReleaseStart | SampleStart -- SampleEnd |
| LowerVelocityXFade | 0 -- (HiVel - LoVel) |
| UpperVelocityXFade | 0 -- (HiVel - LoVel) |

## Async Properties

From `ModulatorSamplerSound::isAsyncProperty()`:

These properties are considered "async" -- they affect the streaming/playback engine and are handled specially:
- SampleStart, SampleEnd, SampleStartMod
- LoopStart, LoopEnd, LoopXFade, LoopEnabled
- SampleState, ReleaseStart

When `updateInternalData` is called, async properties trigger operations on the underlying `StreamingSamplerSound` objects (preload changes, loop reconfiguration), while non-async properties update internal bitmasks and cached values directly.

## Preprocessor Guards

### deleteSample -- HI_ENABLE_EXPANSION_EDITING

```cpp
void ScriptingObjects::ScriptingSamplerSound::deleteSample()
{
#if HI_ENABLE_EXPANSION_EDITING
    // ... implementation
    handler->getSampler()->killAllVoicesAndCall(f);
#endif
}
```

The entire body of `deleteSample` is guarded by `HI_ENABLE_EXPANSION_EDITING`. In builds without this flag, the method exists but does nothing.

## Threading Patterns

### duplicateSample -- Voice Killing + Sample Lock

```cpp
ScopedValueSetter<bool> svs(s->getSampleMap()->getSyncEditModeFlag(), true);
SuspendHelpers::ScopedTicket ticket(mc);
mc->getJavascriptThreadPool().killVoicesAndExtendTimeOut(jp, 1000);
while (mc->getKillStateHandler().isAudioRunning())
    Thread::sleep(100);
LockHelpers::freeToGo(s->getMainController());
LockHelpers::SafeLock sl(mc, LockHelpers::Type::SampleLock);
```

This is the most heavyweight threading pattern in the class:
1. Sets sync edit mode flag on SampleMap
2. Gets a suspension ticket
3. Kills voices with 1000ms timeout
4. Busy-waits for audio to stop
5. Acquires sample lock
6. Then performs the duplication (copy data, add to sample map, refresh preload)

### deleteSample -- killAllVoicesAndCall

Uses the async `killAllVoicesAndCall` pattern -- schedules removal via a lambda that runs after voices are killed.

### replaceAudioFile -- No explicit threading

Directly writes to audio files. Does NOT use killAllVoicesAndCall. The caller is responsible for ensuring safe state. The method writes through `StreamingSamplerSound::replaceAudioFile()`.

### set/get/getRange/setFromJSON -- No threading guards

These are thin wrappers with no explicit thread safety. The underlying `setSampleProperty` / `getSampleProperty` on ModulatorSamplerSound operate on ValueTree data which has its own thread safety characteristics.

## loadIntoBufferArray -- Multi-Mic Channel Handling

Iterates over all multi-mic samples (`sound->getNumMultiMicSamples()`), creating an AudioFormatReader for each. For mono samples, adds one VariantBuffer; for stereo, adds two. Returns a flat array of all channel buffers across all mic positions.

**Memory implications:** Loads the entire sample into memory as VariantBuffer objects. For large samples or many mic positions, this allocates significant memory.

## replaceAudioFile -- Validation Chain

1. Checks sound exists
2. Validates audioData is array
3. Checks no mic position is monolithic (can't write to monolith files)
4. Counts total channels across mic positions (mono=1, stereo=2 each)
5. Validates buffer count matches total channel count
6. Validates all buffers have same length
7. Writes through `StreamingSamplerSound::replaceAudioFile()` per mic position

## getCustomProperties -- Lazy DynamicObject

Returns a mutable DynamicObject that is lazily created on first access:
```cpp
if (customObject.isObject())
    return customObject;
customObject = var(new DynamicObject());
return customObject;
```

This allows scripts to attach arbitrary key-value metadata to individual samples. The data lives only on the ScriptingSamplerSound wrapper -- it is NOT persisted in the sample map.

## getCachedIndex -- String-to-Index Resolution

```cpp
int getCachedIndex(const var &indexExpression) const
{
    if (indexExpression.isString())
    {
        Identifier thisId(indexExpression.toString());
        auto idx = sampleIds.indexOf(thisId);
        if (idx == -1)
            reportScriptError("Can't find property " + thisId.toString());
        return idx;
    }
    return (int)indexExpression;
}
```

Supports both string identifiers ("Root", "HiKey") and integer constants. This is compiled at parse time -- the index is cached for runtime performance.

## Debug Interface

`getNumChildElements()` returns `sampleIds.size() + (customObject.isObject() ? 1 : 0)`. The debugger shows each sample property as a child element, plus the custom properties object if it exists. `getDebugValue()` returns the FileName property as the summary string.

## Factory / obtainedVia

Sample objects are NOT created directly by scripts. They are produced by:

1. **Sampler.createSelection(regex)** -- returns array of Sample objects matching regex on filename
2. **Sampler.createSelectionFromIndexes(indexOrArray)** -- returns array of Sample objects by index (-1 for all)
3. **Sampler.createSelectionWithFilter(filterFunction)** -- returns array of Sample objects passing filter
4. **Sample.duplicateSample()** -- returns a new Sample object (clone of current)

The constructor takes a `ModulatorSampler*` and `ModulatorSamplerSound::Ptr`, both of which are internal C++ types not directly accessible from script.

## Relationship to Sampler Class

As documented in the Sampler prerequisite:
- **Legacy selection** (Sampler): index-based with `selectSounds()` / `getSoundProperty()` / `setSoundProperty()`
- **Modern selection** (Sample objects): `createSelection*()` returns Sample arrays, each Sample provides direct property access

The Sample class IS the "modern selection" system. The same SampleIds property constants (FileName, Root, HiKey, etc.) are used by both the Sampler's legacy `getSoundProperty()`/`setSoundProperty()` and Sample's `get()`/`set()`.

The Sampler Readme lists constants FileName=1 through NumQuarters=24. The Sample class only exposes FileName=1 through Reversed=23 -- it does NOT include NumQuarters. This is because NumQuarters is added to the Sampler's sampleIds but not to Sample's.

## SampleIds Namespace

**File:** `HISE/hi_core/hi_sampler/sampler/ModulatorSamplerSound.h:118-171`

The full SampleIds namespace includes properties beyond what Sample exposes:
- `Unused`, `ID` -- not exposed as constants
- `NormalizedPeak` -- not in Sample's sampleIds
- `GainTable`, `PitchTable`, `LowPassTable` -- table envelope data, not in Sample's sampleIds
- `NumQuarters` -- timestretching length, not in Sample's sampleIds (only in Sampler)

`SampleIds::numProperties = 25` is used as the constant slot count in the constructor.

## ModulatorSamplerSound Property Behavior

### Async vs Sync Properties

Async properties (SampleStart, SampleEnd, SampleStartMod, LoopStart, LoopEnd, LoopXFade, LoopEnabled, SampleState, ReleaseStart) trigger operations on the streaming layer when modified.

Non-async properties (Root, HiKey, LoKey, HiVel, LoVel, RRGroup, Volume, Pan, etc.) update internal bitmask caches and cached values directly in `updateInternalData()`.

### Value Clipping

`setSampleProperty()` automatically clips values to the valid range returned by `getPropertyRange()`. Scripts don't need to validate values before calling `set()` -- out-of-range values are silently clamped.

### Cascading Range Adjustments

When setting certain properties, `clipRangeProperties()` adjusts dependent properties:
- Setting SampleStart may adjust LoopXFade, LoopStart, SampleStartMod
- Setting SampleEnd may adjust LoopXFade, LoopEnd, ReleaseStart
- Setting LoopStart may adjust LoopEnd
- Setting LoopEnd may adjust LoopStart, ReleaseStart

This ensures internal consistency of the sample's range properties.
