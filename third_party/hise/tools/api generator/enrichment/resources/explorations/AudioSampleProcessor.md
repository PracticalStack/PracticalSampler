# AudioSampleProcessor -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- prerequisite table
- `enrichment/resources/survey/class_survey_data.json` -- factory/seeAlso data
- `enrichment/phase1/Synth/Readme.md` -- prerequisite class (Synth)
- C++ source files listed below

## Source Files

- **Scripting wrapper header:** `hi_scripting/scripting/api/ScriptingApiObjects.h:2419-2498`
- **Scripting wrapper impl:** `hi_scripting/scripting/api/ScriptingApiObjects.cpp:4763-5017`
- **C++ base class header:** `hi_core/hi_dsp/ProcessorInterfaces.h:334-407`
- **C++ base class impl:** `hi_core/hi_dsp/ProcessorInterfaces.cpp:724-811`
- **setLoadedFile impl:** `hi_core/hi_dsp/Processor.cpp:1322-1379+`
- **Factory method (Synth):** `hi_scripting/scripting/api/ScriptingApi.cpp:6092-6113`
- **AudioLooper (implementor):** `hi_core/hi_modules/synthesisers/synths/AudioLooper.h`
- **ConvolutionEffect (implementor):** `hi_core/hi_modules/effects/fx/Convolution.h`
- **NoiseGrainPlayer (implementor):** `hi_core/hi_modules/effects/fx/NoiseGrainPlayer.h`
- **WavetableSynth (implementor):** `hi_core/hi_modules/synthesisers/synths/WavetableSynth.h`

---

## Class Declaration

```cpp
// ScriptingApiObjects.h:2419
class ScriptingAudioSampleProcessor : public ConstScriptingObject
{
public:
    ScriptingAudioSampleProcessor(ProcessorWithScriptingContent *p, Processor *sampleProcessor);
    ~ScriptingAudioSampleProcessor() {};

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("AudioSampleProcessor"); }
    Identifier getObjectName() const override { return getClassName(); };
    bool objectDeleted() const override { return audioSampleProcessor.get() == nullptr; }
    bool objectExists() const override { return audioSampleProcessor != nullptr; }

    // ... API methods ...

private:
    WeakReference<Processor> audioSampleProcessor;
};
```

The scripting class is a thin handle wrapping a `WeakReference<Processor>`. It casts to `ProcessorWithExternalData*` when it needs to access the audio file data. The handle can become invalid (deleted module), detected via `objectDeleted()`.

---

## Inheritance Chain

### Scripting Wrapper
`ScriptingAudioSampleProcessor` -> `ConstScriptingObject` -> `ApiClass` + `DynamicObject`

### C++ Base Class (what it wraps)
The underlying processor implements:
`AudioSampleProcessor` -> `ProcessorWithSingleStaticExternalData` -> `ProcessorWithExternalData` -> `ExternalDataHolder`

Also: `AudioSampleProcessor` : `PoolBase::Listener` (for pool reload notifications)

`ProcessorWithSingleStaticExternalData` manages exactly one complex data object of a given type. For AudioSampleProcessor, this is `ExternalData::DataType::AudioFile` with count 1.

---

## Constructor Analysis

```cpp
// ScriptingApiObjects.cpp:4786
ScriptingAudioSampleProcessor(ProcessorWithScriptingContent *p, Processor *sampleProcessor) :
    ConstScriptingObject(p, dynamic_cast<Processor*>(sampleProcessor) != nullptr 
                         ? dynamic_cast<Processor*>(sampleProcessor)->getNumParameters() : 0),
    audioSampleProcessor(dynamic_cast<Processor*>(sampleProcessor))
{
    if (audioSampleProcessor != nullptr)
    {
        setName(audioSampleProcessor->getId());
        for (int i = 0; i < audioSampleProcessor->getNumParameters(); i++)
        {
            addConstant(audioSampleProcessor->getIdentifierForParameterIndex(i).toString(), var(i));
        }
    }
    else
    {
        setName("Invalid Processor");
    }

    ADD_API_METHOD_2(setAttribute);
    ADD_API_METHOD_1(getAttribute);
    ADD_API_METHOD_1(getAttributeId);
    ADD_API_METHOD_1(getAttributeIndex);
    ADD_API_METHOD_0(getNumAttributes);
    ADD_API_METHOD_1(setBypassed);
    ADD_API_METHOD_0(isBypassed);
    ADD_API_METHOD_0(getSampleLength);
    ADD_API_METHOD_2(setSampleRange);
    ADD_API_METHOD_0(getSampleRange);
    ADD_API_METHOD_0(getTotalLengthInSamples);
    ADD_API_METHOD_1(getLoopRange);
    ADD_API_METHOD_1(setFile);
    ADD_API_METHOD_1(getAudioFile);
    ADD_API_METHOD_0(getFilename);
    ADD_API_METHOD_0(getSampleStart);
}
```

### Dynamic Constants

The constructor uses `addConstant()` in a loop to expose the wrapped processor's parameter names as integer constants. These are NOT fixed -- they depend on which concrete processor type is wrapped. Examples by module type:

**AudioLooper (AudioLoopPlayer):**
| Name | Value | Description |
|------|-------|-------------|
| Gain | 0 | ModulatorSynth base |
| Balance | 1 | ModulatorSynth base |
| VoiceLimit | 2 | ModulatorSynth base |
| KillFadeTime | 3 | ModulatorSynth base |
| SyncMode | 4 | Sync to host tempo mode |
| LoopEnabled | 5 | Enable loop playback |
| PitchTracking | 6 | Track pitch to MIDI note |
| RootNote | 7 | Root note for pitch tracking |
| SampleStartMod | 8 | Sample start modulation |
| Reversed | 9 | Play in reverse |

**ConvolutionEffect:**
| Name | Value | Description |
|------|-------|-------------|
| DryGain | 0 | Gain of unprocessed input |
| WetGain | 1 | Gain of convolved input |
| Latency | 2 | Latency (unused) |
| ImpulseLength | 3 | Deprecated |
| ProcessInput | 4 | Reset on change |
| UseBackgroundThread | 5 | Background thread for IR tail |
| Predelay | 6 | Reverb predelay in ms |
| HiCut | 7 | LP filter on IR |
| Damping | 8 | Fade-out on IR |
| FFTType | 9 | FFT implementation |

**NoiseGrainPlayer:**
| Name | Value | Description |
|------|-------|-------------|
| Position | 0 | Playback position |
| Mix | 1 | Wet/dry mix |
| WhiteNoise | 2 | White noise amount |
| GrainSize | 3 | Grain size (1-6) |

### Method Registration

All methods use `ADD_API_METHOD_N` (untyped). No `ADD_TYPED_API_METHOD_N` registrations found. All 16 API methods are plain untyped.

---

## Factory Method / obtainedVia

### Synth.getAudioSampleProcessor(name)

```cpp
// ScriptingApi.cpp:6092
ScriptingObjects::ScriptingAudioSampleProcessor * ScriptingApi::Synth::getAudioSampleProcessor(const String &name)
{
    WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);
    
    Processor::Iterator<ProcessorWithExternalData> it(owner);
    ProcessorWithExternalData *asp;
    
    while ((asp = it.getNextProcessor()) != nullptr)
    {
        if (dynamic_cast<Processor*>(asp)->getId() == name)
        {
            if (asp->getNumDataObjects(ExternalData::DataType::AudioFile) > 0)
            {
                return new ScriptAudioSampleProcessor(getScriptProcessor(), dynamic_cast<Processor*>(asp));
            }
        }
    }
    
    reportScriptError(name + " was not found. ");
    RETURN_IF_NO_THROW(new ScriptAudioSampleProcessor(getScriptProcessor(), nullptr))
}
```

Key observations:
1. Uses `Processor::Iterator<ProcessorWithExternalData>` -- searches for ANY processor with external data, not just `AudioSampleProcessor` C++ class
2. Filters by `getNumDataObjects(ExternalData::DataType::AudioFile) > 0` -- must have at least one audio file slot
3. Owner-rooted search (subtree only, per Synth prerequisite Readme)
4. `WARN_IF_AUDIO_THREAD` -- onInit only (object creation restriction)
5. Reports script error if not found, returns invalid handle in no-throw mode

Also created by `Builder` (per survey data).

---

## C++ Base Class: AudioSampleProcessor

```cpp
// ProcessorInterfaces.h:334
class AudioSampleProcessor: public PoolBase::Listener,
                            public ProcessorWithSingleStaticExternalData
{
public:
    enum SyncToHostMode
    {
        FreeRunning = 1,
        OneBeat,
        TwoBeats,
        OneBar,
        TwoBars,
        FourBars,
        EightBars,
        TwelveBars,
        SixteenBars
    };
    
    AudioSampleProcessor(MainController* mc);
    virtual ~AudioSampleProcessor();
    
    void saveToValueTree(ValueTree &v) const;
    void restoreFromValueTree(const ValueTree &v);
    void setLoadedFile(const String &fileName, bool loadThisFile = false, bool forceReload = false);
    void poolEntryReloaded(PoolReference referenceThatWasChanged) override;
    String getFileName() const;
    double getSampleRateForLoadedFile() const;
    AudioSampleBuffer& getAudioSampleBuffer();
    const AudioSampleBuffer& getAudioSampleBuffer() const;
    MultiChannelAudioBuffer& getBuffer();
    const MultiChannelAudioBuffer& getBuffer() const;
    MultiChannelAudioBuffer* getAudioFileUnchecked(int index = 0);
    const MultiChannelAudioBuffer* getAudioFileUnchecked(int index = 0) const;
    
protected:
    WeakReference<AudioSampleBufferPool> currentPool;
    
private:
    int getConstrainedLoopValue(String metadata);
};
```

### Constructor Implementation

```cpp
// ProcessorInterfaces.cpp:724
AudioSampleProcessor::AudioSampleProcessor(MainController* mc):
    ProcessorWithSingleStaticExternalData(mc, ExternalData::DataType::AudioFile, 1)
{
    currentPool = &mc->getActiveFileHandler()->pool->getAudioSampleBufferPool();
}
```

Initializes with exactly 1 AudioFile data object. Registers with the active audio sample buffer pool.

---

## SyncToHostMode Enum

```cpp
enum SyncToHostMode
{
    FreeRunning = 1,  // No sync
    OneBeat,          // = 2
    TwoBeats,         // = 3
    OneBar,           // = 4
    TwoBars,          // = 5
    FourBars,         // = 6
    EightBars,        // = 7
    TwelveBars,       // = 8
    SixteenBars       // = 9
};
```

This enum is NOT exposed as scripting constants. It is used internally by `AudioLooper` as parameter `SyncMode`. The AudioLooper exposes it as an attribute that can be set via `setAttribute()` using the dynamic constant index.

---

## Known Implementing Processor Types

The C++ `AudioSampleProcessor` interface is implemented by:

| Processor | SET_PROCESSOR_NAME | Type | Notes |
|-----------|-------------------|------|-------|
| AudioLooper | "AudioLooper" / "Audio Loop Player" | ModulatorSynth | Primary use case. Plays single audio file with loop, pitch tracking, tempo sync |
| ConvolutionEffect | "Convolution" / "Convolution Reverb" | MasterEffectProcessor | Uses audio file as impulse response |
| NoiseGrainPlayer | "NoiseGrainPlayer" / "Noise Grain Player" | VoiceEffectProcessor | Granular noise residual playback |
| WavetableSynth | "WavetableSynth" / "Wavetable Synthesiser" | ModulatorSynth | Uses audio file for wavetable data |

However, the factory method `Synth.getAudioSampleProcessor()` searches for `ProcessorWithExternalData` with audio file slots, so it can also return handles to other processors that have audio file data objects (beyond the four listed above).

---

## setFile Implementation Details

```cpp
// ScriptingApiObjects.cpp:4887
void ScriptingObjects::ScriptingAudioSampleProcessor::setFile(String fileName)
{
    if (checkValidObject())
    {
#if USE_BACKEND
        auto pool = audioSampleProcessor->getMainController()->getCurrentAudioSampleBufferPool();
        if (!fileName.contains("{EXP::") && !pool->areAllFilesLoaded())
        {
            PoolReference ref(getScriptProcessor()->getMainController_(), fileName, FileHandlerBase::AudioFiles);
            if (ref.getReferenceString().contains("{PROJECT_FOLDER}"))
            {
                reportScriptError("You must call Engine.loadAudioFilesIntoPool() before using this method");
            }
        }
#endif
        auto p = dynamic_cast<ProcessorWithExternalData*>(audioSampleProcessor.get());
        jassert(p != nullptr);
        p->getAudioFile(0)->fromBase64String(fileName);
    }
}
```

Key behaviors:
1. **Backend-only pool check:** In USE_BACKEND mode, if the pool is not fully loaded and the filename uses `{PROJECT_FOLDER}`, it reports a script error requiring `Engine.loadAudioFilesIntoPool()` first
2. **Expansion support:** Filenames containing `{EXP::` bypass the pool check (expansion files are loaded separately)
3. **Delegates to `MultiChannelAudioBuffer::fromBase64String()`** -- the filename is NOT a filesystem path but a pool reference string
4. **Always operates on slot 0** (first audio file)

---

## Sample Range Methods

All range methods delegate to the `MultiChannelAudioBuffer` at slot 0:

- `getSampleStart()` -> `getAudioFile(0)->getCurrentRange().getStart()`
- `getSampleRange()` -> returns `[start, end]` array from `getCurrentRange()`
- `getTotalLengthInSamples()` -> `getTotalRange().getEnd()`
- `getSampleLength()` -> `getCurrentRange().getLength()`
- `setSampleRange(start, end)` -> `getAudioFile(0)->setRange(Range<int>(start, end))`
- `getLoopRange(subtractStart)` -> `getAudioFile(0)->getLoopRange(subtractStart)` returns `[start, end]`

---

## getAudioFile Method

```cpp
// ScriptingApiObjects.cpp:4997
var ScriptingObjects::ScriptingAudioSampleProcessor::getAudioFile(int slotIndex)
{
    if (checkValidObject())
    {
        if (auto ed = dynamic_cast<ProcessorWithExternalData*>(audioSampleProcessor.get()))
            return var(new ScriptAudioFile(getScriptProcessor(), slotIndex, ed));
    }
    reportScriptError("Not a valid object");
    RETURN_IF_NO_THROW(var());
}
```

Creates and returns a `ScriptAudioFile` (the `AudioFile` scripting API class) reference to the given slot index. This bridges from AudioSampleProcessor handle to the complex data AudioFile API.

---

## getFilename Method

```cpp
String ScriptingObjects::ScriptingAudioSampleProcessor::getFilename()
{
    if (checkValidObject())
    {
        if (checkValidObject())  // Note: redundant double-check
        {
            return dynamic_cast<ProcessorWithExternalData*>(audioSampleProcessor.get())->getAudioFile(0)->toBase64String();
        }
    }
    return {};
}
```

Returns the pool reference string (including wildcards like `{PROJECT_FOLDER}` or `{EXP::name}`), not a filesystem path.

---

## Attribute Methods

Standard pattern shared with Effect, ChildSynth, Modulator, etc.:
- `setAttribute(index, value)` -> `audioSampleProcessor->setAttribute(index, value, sendNotificationAsync)`
- `getAttribute(index)` -> `audioSampleProcessor->getAttribute(index)`
- `getAttributeId(index)` -> `audioSampleProcessor->getIdentifierForParameterIndex(index).toString()`
- `getAttributeIndex(id)` -> `audioSampleProcessor->getParameterIndexForIdentifier(id)`
- `getNumAttributes()` -> `audioSampleProcessor->getNumParameters()`

---

## Bypass Methods

- `setBypassed(bool)` -> `audioSampleProcessor->setBypassed(shouldBeBypassed, sendNotification)` + `sendOtherChangeMessage(ProcessorChangeEvent::Bypassed)`
- `isBypassed()` -> `audioSampleProcessor->isBypassed()`

---

## Threading / Lifecycle Constraints

1. **onInit only:** Object creation via `Synth.getAudioSampleProcessor()` is restricted to `onInit` (uses `objectsCanBeCreated()` check in the Synth method, plus `WARN_IF_AUDIO_THREAD`)
2. **WeakReference handle:** The handle holds a `WeakReference<Processor>` so it becomes invalid if the underlying module is deleted (e.g., by Builder)
3. **setFile backend check:** In USE_BACKEND, `setFile` requires `Engine.loadAudioFilesIntoPool()` to have been called first for `{PROJECT_FOLDER}` references (not needed in exported plugins where files are embedded)

---

## Preprocessor Guards

- `#if USE_BACKEND` in `setFile()` -- pool loading validation only in HISE IDE

---

## ValueTree Persistence (C++ base)

```cpp
void AudioSampleProcessor::saveToValueTree(ValueTree &v) const
{
    v.setProperty("FileName", getBuffer().toBase64String(), nullptr);
    v.setProperty("min", sampleRange.getStart(), nullptr);
    v.setProperty("max", sampleRange.getEnd(), nullptr);
    v.setProperty("loopStart", loopRange.getStart(), nullptr);
    v.setProperty("loopEnd", loopRange.getEnd(), nullptr);
}

void AudioSampleProcessor::restoreFromValueTree(const ValueTree &v)
{
    const String savedFileName = v.getProperty("FileName", "");
    getBuffer().fromBase64String(savedFileName);
    setLoadedFile(savedFileName, true);
    auto sRange = Range<int>(v.getProperty("min", 0), v.getProperty("max", 0));
    auto lRange = Range<int>(v.getProperty("loopStart", 0), v.getProperty("loopEnd", 0));
    getBuffer().setRange(sRange);
    getBuffer().setLoopRange(lRange, dontSendNotification);
}
```

Properties saved: FileName (base64/pool ref string), min/max (sample range), loopStart/loopEnd (loop range).

---

## Pool Reload Handling

```cpp
void AudioSampleProcessor::poolEntryReloaded(PoolReference referenceThatWasChanged)
{
    auto s = referenceThatWasChanged.getReferenceString();
    if (getBuffer().toBase64String() == s)
    {
        getBuffer().fromBase64String({});    // clear first
        getBuffer().fromBase64String(s);     // reload
    }
}
```

When a pool entry is reloaded (e.g., file changed on disk), the processor clears and reloads its buffer if it matches the changed reference.
