# WavetableController -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` (WavetableController entry)
- `enrichment/base/WavetableController.json` (8 methods)
- No prerequisite class required
- No base class exploration needed (not a component)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h:2609-2679`

```cpp
struct ScriptWavetableController: public ConstScriptingObject,
                                  public ControlledObject,
                                  public WeakErrorHandler
{
    ScriptWavetableController(ProcessorWithScriptingContent* sp, Processor* wavetableSynth);
    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("WavetableController"); }
    Identifier getObjectName() const override { return getClassName(); };
    bool objectDeleted() const override { return wt_.get() == nullptr; }
    bool objectExists() const override { return wt_.get() != nullptr; }
    // ...
private:
    WavetableSynth* getWavetableSynth() { return dynamic_cast<WavetableSynth*>(wt_.get()); }
    WeakCallbackHolder errorHandler;
    WeakReference<Processor> wt_;
};
```

### Inheritance
- `ConstScriptingObject` -- standard scripting object base (no constants registered -- 0 passed to constructor)
- `ControlledObject` -- provides `getMainController()` access
- `WeakErrorHandler` -- provides `handleErrorMessage()` virtual for error routing

### Error Handling Pattern
The class overrides `handleErrorMessage` to forward errors to a user-supplied JavaScript callback via `WeakCallbackHolder`:
```cpp
void handleErrorMessage(const String& error) override
{
    if(errorHandler)
        errorHandler.call1(error);
}
```
This is connected to the `ResynthesisOptions::errorHandler` field -- `setResynthesisOptions` sets `options.errorHandler = this`.

### Weak Reference Pattern
The underlying `WavetableSynth*` is held via `WeakReference<Processor> wt_`. Every API method checks `if(auto wt = getWavetableSynth())` with `dynamic_cast`, calling `reportScriptError("No wavetable synth")` on failure.

## Factory / obtainedVia

**Factory method:** `Synth.getWavetableController(processorId)`

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp:6293-6302`

```cpp
ScriptingObjects::ScriptWavetableController* ScriptingApi::Synth::getWavetableController(const String& processorId)
{
    auto p = ProcessorHelpers::getFirstProcessorWithName(
        getScriptProcessor()->getMainController_()->getMainSynthChain(), processorId);
    if(auto wt = dynamic_cast<WavetableSynth*>(p))
        return new ScriptingObjects::ScriptWavetableController(getScriptProcessor(), p);
    reportScriptError(processorId + " does not have a routing matrix"); // Note: wrong error message (copy-paste bug)
    RETURN_IF_NO_THROW(new ScriptingObjects::ScriptWavetableController(getScriptProcessor(), nullptr));
}
```

Note: The error message says "does not have a routing matrix" -- this is a copy-paste artifact from the `getRoutingMatrix` method above it. The functional behavior is correct (throws script error).

## Constructor

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp:5197-5213`

```cpp
ScriptWavetableController(ProcessorWithScriptingContent* sp, Processor* wavetableSynth):
    ConstScriptingObject(sp, 0),  // 0 constants
    ControlledObject(sp->getMainController_()),
    wt_(wavetableSynth),
    errorHandler(getScriptProcessor(), this, var(), 1)
{
    ADD_API_METHOD_0(getResynthesisOptions);
    ADD_API_METHOD_1(setResynthesisOptions);
    ADD_API_METHOD_0(resynthesise);
    ADD_API_METHOD_1(saveAsHwt);
    ADD_API_METHOD_1(saveAsAudioFile);
    ADD_API_METHOD_2(setEnableResynthesisCache);
    ADD_API_METHOD_1(setErrorHandler);
    ADD_API_METHOD_3(loadData);
    ADD_API_METHOD_1(setPostFXProcessors);
}
```

**No constants registered.** `ConstScriptingObject(sp, 0)` -- zero constant slots.
**No typed API methods.** All use plain `ADD_API_METHOD_N` -- no forced types.

## Underlying WavetableSynth

**File:** `HISE/hi_core/hi_modules/synthesisers/synths/WavetableSynth.h:251-1088`

```cpp
class WavetableSynth: public ModulatorSynth,
                      public hise::AudioSampleProcessor,
                      public MultiChannelAudioBuffer::Listener,
                      public WaveformComponent::Broadcaster
```

Key infrastructure the API wrapper delegates to:

### SpecialParameters enum
```cpp
enum SpecialParameters {
    HqMode = ModulatorSynth::numModulatorSynthParameters,
    LoadedBankIndex,
    TableIndexValue,
    RefreshMipmap,
    numSpecialParameters
};
```

### ChainIndex enum (modulation chains)
```cpp
enum ChainIndex {
    Gain = 0,
    Pitch = 1,
    TableIndex = 2,
    TableIndexBipolar
};
```

### AudioFileIndex constant
```cpp
static constexpr int AudioFileIndex = 900000;
```
Used internally to distinguish audio-file-based wavetable loading from bank-index-based loading.

### Key methods delegated to by the API wrapper:
- `getResynthesisOptions()` -- returns `resynthOptions` member
- `setResynthesisOptions(options, notification)` -- stores options, optionally triggers reload
- `reloadWavetable()` -- calls `loadWavetableFromIndex(currentBankIndex)`
- `setPostProcessors(Array<PostFXProcessor>&&, notification)` -- swaps postFX list, optionally re-renders
- `setResynthesisCache(File, clearCache)` -- sets cache folder, optionally clears .tmp files
- `getCurrentlyLoadedWavetableTree()` -- returns ValueTree of loaded wavetable
- `createAudioSampleBufferFromWavetable(sampleIndex)` -- reconstructs audio buffer from wavetable data
- `loadWaveTable(ValueTree)` -- loads wavetable from ValueTree into sounds
- `getBuffer()` -- returns `MultiChannelAudioBuffer` reference for loading audio data

## ResynthesisOptions JSON Schema

**File:** `HISE/hi_core/hi_modules/synthesisers/synths/WavetableTools.h:69-102`
**Serialization:** `HISE/hi_core/hi_modules/synthesisers/synths/WavetableTools.cpp:41-97`

### Fields and defaults

| JSON Property | C++ Field | Type | Default | Description |
|---------------|-----------|------|---------|-------------|
| `PhaseMode` | `pm` | String (enum) | `"StaticPhase"` | Phase handling mode |
| `MipMapSize` | `mipMapSize` | int | 12 | Number of semitones per mipmap band |
| `CycleMultiplier` | `cycleMultiplier` | int | 4 | Multiplier for cycle length detection |
| `UseTransientMode` | `useTransientMode` | bool | true | Use transient detection during resynthesis |
| `NumCycles` | `numCycles` | int | -1 | Fixed cycle count (-1 = auto). Clamped to max 512, rounded to next power of two |
| `ForceResynthesis` | `forceResynthesis` | bool | false | Force resynthesis even if cached |
| `UseLoris` | `useLoris` | bool | true (if HISE_INCLUDE_LORIS), false otherwise | Use Loris for resynthesis |
| `ReverseOrder` | `reverseOrder` | bool | false | Reverse cycle order |
| `RemoveNoise` | `removeNoise` | bool | true | Enable noise removal |
| `DenoiseSettings` | `sitranoSettings` | Object | default ConfigData | SiTraNo denoising parameters |
| `RootNote` | `rootNote` | int | -1 | Root note (-1 = auto-detect) |

### PhaseMode enum values
```cpp
enum class PhaseMode {
    Resample,      // Simple resampling
    ZeroPhase,     // Zero-phase alignment
    StaticPhase,   // Static phase (default)
    DynamicPhase   // Dynamic phase per cycle
};
```

### DenoiseSettings sub-object (SiTraNoConverter::ConfigData)
**File:** `HISE/hi_tools/hi_tools/SiTraNoConverter.h:100-117`

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `slowFFTOrder` | int | 13 | Order for slow FFT (8192 samples) |
| `fastFFTOrder` | int | 9 | Order for fast FFT (512 samples) |
| `freqResolution` | double | 500.0 | Frequency resolution for median filter (Hz) |
| `timeResolution` | double | 0.2 | Time resolution for median filter (seconds) |
| `calculateTransients` | bool | true | Whether to separate transients from noise |
| `slowTransientThreshold` | double[2] | {0.8, 0.7} | Thresholds for slow transient detection |
| `fastTransientThreshold` | double[2] | {0.85, 0.75} | Thresholds for fast transient detection |

Note: The `fromVar` for `RemoveNoise` has a bug -- it uses `reverseOrder` as default instead of `removeNoise`:
```cpp
removeNoise = o.getProperty("RemoveNoise", reverseOrder);  // should be removeNoise
```

## PostFXProcessor Infrastructure

**File:** `HISE/hi_core/hi_modules/synthesisers/synths/WavetableSynth.h:263-585`

### PostFXProcessor::Type enum
```cpp
enum class Type {
    Identity = 0,   // No-op passthrough
    Custom,         // Uses connected Table for waveshaping
    Sin,            // Sine waveshaping
    Warp,           // Time-domain warping
    FM1,            // FM modulation (1x carrier)
    FM2,            // FM modulation (2x carrier)
    FM3,            // FM modulation (3x carrier)
    FM4,            // FM modulation (4x carrier)
    Sync,           // Hard sync effect
    Root,           // Additive sine
    Clip,           // Hard clipping
    Tanh,           // Soft saturation
    Bitcrush,       // Bit reduction
    SampleAndHold,  // Sample-and-hold downsampling
    Fold,           // Wavefolder
    Normalise,      // Gain normalization
    Phase,          // Phase rotation
    numTypes
};
```

### PostFXProcessor JSON schema (fromVar)

| JSON Property | Type | Description |
|---------------|------|-------------|
| `Type` | String | One of the type names above |
| `TableProcessor` | String | ID of processor providing Table for Custom type |
| `TableIndex` | int | Index of table within the TableProcessor |
| `min` | double | Parameter range minimum (via RangeHelpers) |
| `max` | double | Parameter range maximum (via RangeHelpers) |
| `skew` | double | Parameter range skew (via RangeHelpers) |
| `step` | double | Parameter range step (via RangeHelpers) |

The range properties (`min`, `max`, `skew`, `step`) are stored via `scriptnode::RangeHelpers::storeDoubleRange` using the `ScriptComponents` ID set.

### PostFXProcessor::apply logic
```cpp
void apply(VariantBuffer::Ptr cycle, double normalisedIndex, NotificationType n)
{
    if(cf)
    {
        // If connected to a table, use table lookup to transform the index
        if(auto ws = dynamic_cast<SampleLookupTable*>(connectedTable.get()))
            normalisedIndex = jlimit(0.0, 1.0, (double)ws->getInterpolatedValue(normalisedIndex, n));
        // Convert normalized 0-1 to parameter range
        auto v = parameterRange.convertFrom0to1(normalisedIndex, true);
        cf(cycle, (float)v);
    }
}
```

The `normalisedIndex` parameter comes from the cycle's position in the wavetable (0.0 to 1.0), allowing PostFX parameters to vary across cycles. The optional Table connection provides additional waveshaping of this index.

### Post-processing pipeline in WavetableSynth

When `setPostProcessors` is called:
1. The new processor list is swapped in under `postFXLock` (CriticalSection)
2. `renderPostFX(true)` is called which uses `killVoicesAndCall` to safely:
   a. Restore the original resynthesised data from `lastResynthesisedData`
   b. Call `postProcessCycles()` -- applies all PostFX to each cycle
   c. Call `rebuildMipMaps()` -- rebuilds wavetable from processed cycles
3. Post-processing normalizes gain across all cycles after processing

## loadData Implementation Details

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp:5321-5367`

The `loadData` method accepts three input types for `bufferOrFile`:

1. **ScriptFile object** -- creates a PoolReference and loads via `buffer.fromBase64String(ref.getReferenceString())`
2. **Array of Buffers** -- multi-channel loading: extracts float pointers from each Buffer in the array, creates AudioSampleBuffer, loads with sampleRate and loopRange
3. **Single Buffer** -- loads directly from buffer with sampleRate and loopRange

The `loopRange` parameter expects an array of exactly 2 elements: `[startSample, endSample]`.

Note: When loading from ScriptFile, the sampleRate and loopRange parameters are not used (the reference string carries its own metadata).

## saveAsAudioFile Implementation Details

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp:5278-5305`

- Creates audio buffer from wavetable data at sample index 0
- Gets loop points from `WavetableSound::getTableSize() - 1`
- Writes WAV format at 48000 Hz, 24-bit
- Sets loop metadata: `Loop0Start=0`, `Loop0End=tableSize-1`, `NumSampleLoops=1`
- The output file is obtained via `ScriptingApi::FileSystem::getFileFromVar`

## saveAsHwt Implementation Details

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp:5255-5276`

- Gets `currentlyLoadedWavetableTree` ValueTree from the synth
- Expects a ScriptFile object (uses `dynamic_cast<ScriptFile*>`)
- Writes ValueTree binary format to file via `v.writeToStream(fos)`
- HWT = HISE Wavetable format (proprietary binary ValueTree)

## Resynthesis Cache Mechanism

**File:** `HISE/hi_core/hi_modules/synthesisers/synths/WavetableSynth.h:889-974`

Cache files are stored as `.tmp` files in the specified directory. The filename is computed as:
```
hash(filename) + "_" + hash(JSON(resynthOptions)) + ".tmp"
```

This means cache is invalidated when either the source filename or the resynthesis options change.

- `loadFromCache()` scans `.tmp` files in the cache folder for matching hash
- `storeToCache()` saves the `ExportData` MemoryBlock to a `.tmp` file
- `clearCache` parameter deletes all `.tmp` files in the cache folder

## Preprocessor Guards

- `HISE_INCLUDE_LORIS` -- guards `useLoris` default value in ResynthesisOptions. When Loris is not available, `useLoris` defaults to false and the `UseLoris` property in `fromVar` is silently ignored.
- No `USE_BACKEND` guards in the ScriptWavetableController wrapper itself -- all methods available in frontend and backend.

## Threading Considerations

- `setPostProcessors` uses `CriticalSection postFXLock` for thread safety
- Post-FX rendering uses `killVoicesAndCall` to safely process on the SampleLoadingThread
- `resynthesise` calls `reloadWavetable()` which calls `loadWavetableFromIndex()` -- this is a heavy operation (FFT, SiTraNo decomposition) that runs on the sample loading thread
- All API methods check for null wavetable synth before proceeding
