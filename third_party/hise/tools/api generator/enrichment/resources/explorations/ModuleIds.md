# ModuleIds -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- no prerequisites for ModuleIds
- `enrichment/resources/survey/class_survey_data.json` -- ModuleIds not present (constants-only namespace)
- `enrichment/base/ModuleIds.json` -- confirms zero methods, category "namespace"

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.h` lines 1465-1480

```cpp
/** A list with all available modules. */
class ModuleIds : public ApiClass
{
public:
    ModuleIds(ModulatorSynth* s);

    /** Returns the name. */
    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("ModuleIds"); }

private:
    static Array<Identifier> getTypeList(ModulatorSynth* s);
    ModulatorSynth* ownerSynth;
};
```

**Inheritance:** `ApiClass` (from `JavascriptApiClass.h` line 350). ApiClass extends
`ReferenceCountedObject` and `DebugableObjectBase`. It provides the constant registration
mechanism via `addConstant(String constantName, var value)`.

**Key points:**
- No methods at all -- zero `ADD_API_METHOD_N` registrations
- All functionality is via dynamically registered constants
- Constructor takes a `ModulatorSynth*` (the owner synth of the script processor)
- Static helper `getTypeList()` collects all available processor type identifiers

## Constructor and Constant Registration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` lines 7353-7365

```cpp
ScriptingApi::ModuleIds::ModuleIds(ModulatorSynth* s):
    ApiClass(getTypeList(s).size()),  // pre-allocates constant storage
    ownerSynth(s)
{
    auto list = getTypeList(ownerSynth);
    list.sort();  // alphabetical sort

    for (int i = 0; i < list.size(); i++)
    {
        addConstant(list[i].toString(), list[i].toString());
    }
}
```

**Key observations:**
- `ApiClass(N)` constructor takes the number of constants to pre-allocate
- Constants are name=value pairs where BOTH are the type identifier string
  (e.g., `addConstant("LFO", "LFO")`)
- The list is sorted alphabetically before registration
- Each constant's name IS its value -- this is a string-to-string identity mapping
- In HiseScript, accessed as `ModuleIds.LFO`, which evaluates to the string `"LFO"`

## getTypeList -- The Collection Mechanism

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` lines 7367-7392

```cpp
Array<Identifier> ScriptingApi::ModuleIds::getTypeList(ModulatorSynth* s)
{
    Array<Identifier> ids;

    for (int i = 0; i < s->getNumInternalChains(); i++)
    {
        Chain* c = dynamic_cast<Chain*>(s->getChildProcessor(i));
        jassert(c != nullptr);

        if (c != nullptr)
        {
            FactoryType* t = c->getFactoryType();
            auto list = t->getAllowedTypes();

            for (int j = 0; j < list.size(); j++)
            {
                ids.addIfNotAlreadyThere(list[j].type);
            }
        }
    }

    return ids;
}
```

**Algorithm:**
1. Iterate all internal chains of the owner synth
2. For each chain, get its `FactoryType`
3. From each factory, get the `getAllowedTypes()` list (respects constrainers)
4. Collect all unique `ProcessorEntry.type` identifiers (deduplicates with `addIfNotAlreadyThere`)

This means the available constants depend on:
- Which synth type owns the script processor
- What chains that synth has
- What constrainers are active on each chain's factory

## Registration in Script Processor

**File:** `HISE/hi_scripting/scripting/ScriptProcessorModules.cpp` line 302

```cpp
scriptEngine->registerApiClass(new ScriptingApi::ModuleIds(getOwnerSynth()));
```

Registered as part of `JavascriptMidiProcessor` initialization, alongside other API
namespaces (Message, Engine, Synth, Sampler, etc.). The `getOwnerSynth()` returns the
`ModulatorSynth` that contains this script processor in its module tree.

## ModulatorSynth Internal Chains

**File:** `HISE/hi_core/hi_dsp/modules/ModulatorSynth.h` lines 94-101

```cpp
enum InternalChains
{
    MidiProcessor = 0,
    GainModulation,
    PitchModulation,
    EffectChain,
    numInternalChains  // = 4
};
```

The base `ModulatorSynth` has 4 internal chains. Subclasses may override
`getNumInternalChains()` to add more (e.g., `WavetableSynth`, `ModulatorSampler`).

Each chain has its own `FactoryType` subclass that determines which processor types
can be added to it.

## FactoryType System

**File:** `HISE/hi_core/hi_dsp/ProcessorInterfaces.h` lines 938-1067

`FactoryType` is the abstract base for processor factories. Key members:
- `ProcessorEntry` struct: holds `Identifier type` and `String name`
- `getAllowedTypes()` -- returns entries filtered by any active `Constrainer`
- `getTypeNames()` -- pure virtual, returns the raw type list
- `createProcessor(int typeIndex, const String& id)` -- pure virtual factory method

**`ADD_NAME_TO_TYPELIST` macro** (ProcessorInterfaces.h line 920):
```cpp
#define ADD_NAME_TO_TYPELIST(x) \
    (typeNames.add(FactoryType::ProcessorEntry(x::getClassType(), x::getClassName())))
```

This uses `SET_PROCESSOR_NAME` which defines both `getClassType()` (the Identifier) and
`getClassName()` (the human-readable name):

```cpp
#define SET_PROCESSOR_NAME(type, name, unused) \
    static String getClassName() {return name;}; \
    static Identifier getClassType() {return Identifier(type);} \
    ...
```

The `type` parameter (first arg to `SET_PROCESSOR_NAME`) is what becomes the constant
value in `ModuleIds`.

## Chain-to-Factory Mapping

### MidiProcessor Chain (index 0)
**Factory:** `MidiProcessorFactoryType` (MidiProcessor.h line 349)

| C++ Class | Type ID | Display Name |
|-----------|---------|-------------|
| JavascriptMidiProcessor | ScriptProcessor | Script Processor |
| Transposer | Transposer | Transposer |
| MidiPlayer | MidiPlayer | MIDI Player |
| ChokeGroupProcessor | ChokeGroupProcessor | Choke Group Processor |

**Hardcoded MIDI scripts** (via nested `HardcodedScriptFactoryType`):

| C++ Class | Type ID | Display Name |
|-----------|---------|-------------|
| LegatoWithRetrigger | LegatoWithRetrigger | Legato with Retrigger |
| CCSwapper | CCSwapper | CC Swapper |
| ReleaseTrigger | ReleaseTrigger | Release Trigger |
| CC2Note | CC2Note | MIDI CC to Note Generator |
| ChannelFilter | ChannelFilter | MIDI Channel Filter |
| ChannelSetter | ChannelSetter | MIDI Channel Setter |
| MidiMuter | MidiMuter | MidiMuter |
| Arpeggiator | Arpeggiator | Arpeggiator |

### GainModulation Chain (index 1) and PitchModulation Chain (index 2)
**Factory:** `ModulatorChainFactoryType` (ModulatorChain.h line 1065)
Aggregates three sub-factories: VoiceStartModulatorFactoryType, TimeVariantModulatorFactoryType, EnvelopeModulatorFactoryType.

**Voice Start Modulators:**

| C++ Class | Type ID | Display Name |
|-----------|---------|-------------|
| ConstantModulator | Constant | Constant |
| VelocityModulator | Velocity | Velocity Modulator |
| KeyModulator | KeyNumber | Notenumber Modulator |
| RandomModulator | Random | Random Modulator |
| GlobalVoiceStartModulator | GlobalVoiceStartModulator | Global Voice Start Modulator |
| GlobalStaticTimeVariantModulator | GlobalStaticTimeVariantModulator | Global Static Time Variant Modulator |
| ArrayModulator | ArrayModulator | Array Modulator |
| JavascriptVoiceStartModulator | ScriptVoiceStartModulator | Script Voice Start Modulator |
| EventDataModulator | EventDataModulator | Event Data Modulator |

**Time Variant Modulators:**

| C++ Class | Type ID | Display Name |
|-----------|---------|-------------|
| LfoModulator | LFO | LFO Modulator |
| ControlModulator | MidiController | Midi Controller |
| PitchwheelModulator | PitchWheel | Pitch Wheel Modulator |
| MacroModulator | MacroModulator | Macro Control Modulator |
| GlobalTimeVariantModulator | GlobalTimeVariantModulator | Global Time Variant Modulator |
| JavascriptTimeVariantModulator | ScriptTimeVariantModulator | Script Time Variant Modulator |
| HardcodedTimeVariantModulator | HardcodedTimevariantModulator | Hardcoded Time Variant Modulator |

**Envelope Modulators:**

| C++ Class | Type ID | Display Name |
|-----------|---------|-------------|
| SimpleEnvelope | SimpleEnvelope | Simple Envelope |
| AhdsrEnvelope | AHDSR | AHDSR Envelope |
| TableEnvelope | TableEnvelope | Table Envelope |
| JavascriptEnvelopeModulator | ScriptEnvelopeModulator | Script Envelope Modulator |
| MPEModulator | MPEModulator | MPE Modulator |
| ScriptnodeVoiceKiller | ScriptnodeVoiceKiller | Scriptnode Voice Killer |
| GlobalEnvelopeModulator | GlobalEnvelopeModulator | Global Envelope Modulator |
| EventDataEnvelope | EventDataEnvelope | EventData Envelope |
| HardcodedEnvelopeModulator | HardcodedEnvelopeModulator | Hardcoded Envelope Modulator |
| MatrixModulator | MatrixModulator | Matrix Modulator |
| FlexAhdsrEnvelope | FlexAHDSR | Flex AHDSR Envelope |

### EffectChain (index 3)
**Factory:** `EffectProcessorChainFactoryType` (EffectProcessorChain.h line 356)

| C++ Class | Type ID | Display Name |
|-----------|---------|-------------|
| PolyFilterEffect | PolyphonicFilter | Filter |
| HarmonicFilter | HarmonicFilter | Harmonic Filter |
| HarmonicMonophonicFilter | HarmonicFilterMono | Harmonic Filter Monophonic |
| CurveEq | CurveEq | Parametriq EQ |
| StereoEffect | StereoFX | Stereo FX |
| SimpleReverbEffect | SimpleReverb | Simple Reverb |
| GainEffect | SimpleGain | Simple Gain |
| ConvolutionEffect | Convolution | Convolution Reverb |
| DelayEffect | Delay | Delay |
| ChorusEffect | Chorus | Chorus |
| PhaseFX | PhaseFX | Phase FX |
| RouteEffect | RouteFX | Routing Matrix |
| SendEffect | SendFX | Send Effect |
| SaturatorEffect | Saturator | Saturator |
| JavascriptMasterEffect | ScriptFX | Script FX |
| JavascriptPolyphonicEffect | PolyScriptFX | Polyphonic Script FX |
| SlotFX | SlotFX | Effect Slot |
| EmptyFX | EmptyFX | Empty |
| DynamicsEffect | Dynamics | Dynamics |
| AnalyserEffect | Analyser | Analyser |
| ShapeFX | ShapeFX | Shape FX |
| PolyshapeFX | PolyshapeFX | Polyshape FX |
| HardcodedMasterFX | HardcodedMasterFX | Hardcoded Master FX |
| HardcodedPolyphonicFX | HardcodedPolyphonicFX | Hardcoded Polyphonic FX |
| MidiMetronome | MidiMetronome | MidiMetronome |
| NoiseGrainPlayer | NoiseGrainPlayer | Noise Grain Player |

## Additional Synth Types (NOT in ModuleIds)

These are registered in `ModulatorSynthChainFactoryType` but are NOT part of ModuleIds
because ModuleIds only iterates the internal chains of the owner synth, not the synth
chain factory itself. Synth types are children of `ModulatorSynthChain`, not of any
internal chain.

| C++ Class | Type ID | Display Name |
|-----------|---------|-------------|
| ModulatorSampler | StreamingSampler | Sampler |
| SineSynth | SineSynth | Sine Wave Generator |
| ModulatorSynthChain | SynthChain | Container |
| GlobalModulatorContainer | GlobalModulatorContainer | Global Modulator Container |
| WaveSynth | WaveSynth | Waveform Generator |
| NoiseSynth | Noise | Noise Generator |
| WavetableSynth | WavetableSynth | Wavetable Synthesiser |
| AudioLooper | AudioLooper | Audio Loop Player |
| ModulatorSynthGroup | SynthGroup | Synthesiser Group |
| JavascriptSynthesiser | ScriptSynth | Scriptnode Synthesiser |
| MacroModulationSource | MacroModulationSource | Macro Modulation Source |
| SendContainer | SendContainer | Send Container |
| SilentSynth | SilentSynth | Silent Synth |
| HardcodedSynthesiser | HardcodedSynth | Hardcoded Synthesiser |

**Important:** The synth type IDs above are used with `Builder.create()` which takes
`Builder.SynthChain`, `Builder.SynthGroup`, etc. -- NOT from `ModuleIds`. The `Builder`
class has its own dynamic constant system that populates constants from the
`ModulatorSynthChainFactoryType` for synth types plus all chain factories.

## Consumers of ModuleIds Constants

The primary consumer is `Synth.addModule(chainIndex, type, id)` which calls
`ApiHelpers::ModuleHandler::addModule(Chain*, const String& type, const String& id)`.

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` line 6146-6195

The `addModule` method:
1. Checks if a processor with the given ID already exists in the chain (returns it if so)
2. Suspends audio processing via `ScopedTicket`
3. Calls `MainController::createProcessor(c->getFactoryType(), type, id)` which uses
   the chain's own factory type to instantiate the processor
4. Adds the new processor to the chain via `c->getHandler()->add(p, sibling)`

**Usage pattern in HiseScript:**
```javascript
// Add an LFO to the gain modulation chain
Synth.addModule(Synth.Chain.Gain, ModuleIds.LFO, "myLFO");

// Add a filter to the effect chain
Synth.addModule(Synth.Chain.FX, ModuleIds.PolyphonicFilter, "myFilter");
```

The `Builder` class also consumes module type strings but uses its own constant
system rather than referencing `ModuleIds` directly.

## Constrainer Impact on Available Types

The `FactoryType::getAllowedTypes()` method (ProcessorInterfaces.h line 1048) applies
any active constrainer before returning the type list. This means:
- `ModuleIds` respects constrainers that may filter out certain processor types
- Different synth types might have different constrainers active on their chains
- The set of constants is determined at construction time and is static afterward

## Threading and Lifecycle

- `ModuleIds` is constructed once during `JavascriptMidiProcessor` initialization
- The constant list is fixed after construction -- no dynamic updates
- Constants are resolved at compile time by the script engine for performance
- No threading concerns since all data is immutable after construction

## Preprocessor Guards

None. The `ModuleIds` class has no preprocessor guards -- it is available in all build
targets (backend, frontend, DLL).
