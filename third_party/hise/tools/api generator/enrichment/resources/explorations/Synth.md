# Synth -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- prerequisite table, Synth is a prerequisite for 9+ classes
- `enrichment/resources/survey/class_survey_data.json` -- Synth entry: createdBy, creates, seeAlso
- `enrichment/base/Synth.json` -- 61 API methods, category "namespace"
- No prerequisite class for Synth itself (it IS the prerequisite for the module-tree group)

## Source Locations

- **Header:** `HISE/hi_scripting/scripting/api/ScriptingApi.h` lines 1091-1366
- **Implementation:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` lines 5276-6687
- **Owner class:** `HISE/hi_core/hi_dsp/modules/ModulatorSynth.h` (the `owner` pointer type)
- **Script processor modules:** `HISE/hi_scripting/scripting/ScriptProcessorModules.h/.cpp` (host processor types)
- **ModuleHandler:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` lines 46-70
- **ModuleIds:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` lines 7353-7392

---

## Class Declaration

```cpp
class Synth: public ScriptingObject,
             public ApiClass
```

**Key typedefs inside the class:**
```cpp
typedef ScriptingObjects::ScriptingModulator ScriptModulator;
typedef ScriptingObjects::ScriptingEffect ScriptEffect;
typedef ScriptingObjects::ScriptingMidiProcessor ScriptMidiProcessor;
typedef ScriptingObjects::ScriptingSynth ScriptSynth;
typedef ScriptingObjects::ScriptingAudioSampleProcessor ScriptAudioSampleProcessor;
typedef ScriptingObjects::ScriptingTableProcessor ScriptTableProcessor;
typedef ScriptingObjects::ScriptSliderPackProcessor ScriptSliderPackProcessor;
typedef ScriptingObjects::ScriptingSlotFX ScriptSlotFX;
typedef ScriptingObjects::ScriptedMidiPlayer ScriptMidiPlayer;
typedef ScriptingObjects::ScriptRoutingMatrix ScriptRoutingMatrix;
```

These typedefs define the 10 wrapper types that Synth's `get*()` methods return. Each wraps a C++ processor class with a scripting interface.

**Category:** `namespace` (an ApiClass, not a ConstScriptingObject -- it's a global namespace object, not something you create with a factory)

---

## Constructor

```cpp
ScriptingApi::Synth::Synth(ProcessorWithScriptingContent *p, 
                            Message* messageObject_, 
                            ModulatorSynth *ownerSynth)
```

**Constructor parameters:**
- `p` -- The ProcessorWithScriptingContent (the script processor that owns this Synth object)
- `messageObject_` -- Pointer to the Message object for MIDI event tracking
- `ownerSynth` -- The ModulatorSynth that this script processor resides in (the "parent synth")

**Member initialization:**
```cpp
ScriptingObject(p),
ApiClass(0),  // NOTE: ApiClass(0) means NO constants (unlike Engine which has many)
moduleHandler(dynamic_cast<Processor*>(p), dynamic_cast<JavascriptProcessor*>(p)),
messageObject(messageObject_),
owner(ownerSynth),
numPressedKeys(0),
keyDown(0),
sustainState(false),
parentMidiProcessor(dynamic_cast<ScriptBaseMidiProcessor*>(p)),
jp(dynamic_cast<JavascriptMidiProcessor*>(p))
```

**Critical: `ApiClass(0)` -- Synth has NO addConstant() calls.** The class has zero scripting constants. The only constant-like object is `ModuleIds`, which is registered as a separate ApiClass.

**Body:**
- `keyDown.setRange(0, 128, false)` -- initializes 128-bit BigInteger for note tracking
- All 61 methods registered with `ADD_API_METHOD_N` (no `ADD_TYPED_API_METHOD_N` used anywhere)
- Diagnostic checks registered for module lookup methods (backend only)

---

## NO Constants

Unlike Engine, FileSystem, or Colours, the Synth class has **zero** `addConstant()` calls. It is instantiated with `ApiClass(0)`.

The `ModuleIds` class is registered as a separate ApiClass in `registerApiClasses()`, not as part of Synth's constants. It dynamically generates constants based on the owner synth's factory types (see "ModuleIds Class" section below).

---

## Private Members

```cpp
WeakReference<Message> messageObject;       // MIDI message proxy
ModulatorSynth * const owner;              // The parent synth (NEVER null after construction)
Atomic<int> numPressedKeys;                // Track pressed key count
BigInteger keyDown;                        // 128-bit bitfield, one per MIDI note
ApiHelpers::ModuleHandler moduleHandler;   // Handles add/remove module operations
SelectedItemSet<WeakReference<ModulatorSamplerSound>> soundSelection;  // (unused legacy?)
ScriptBaseMidiProcessor* parentMidiProcessor = nullptr;  // Cast of p for MIDI operations
JavascriptMidiProcessor* jp = nullptr;     // Cast of p for deferred callbacks
bool sustainState;                         // Sustain pedal tracking
```

**Critical relationship:** `owner` is a raw `ModulatorSynth * const` -- set once, never changes. All module tree operations are relative to this `owner` as root.

**Private method:**
```cpp
int internalAddNoteOn(int channel, int noteNumber, int velocity, int timestamp, int startOffset);
```
This is the shared implementation for both `playNote()` and `playNoteWithStartOffset()`.

---

## Module Tree Navigation Model

### Two Search Roots

The `get*()` methods use **two different search strategies**, which is one of the most important architectural details for downstream classes:

#### 1. Owner-rooted search (subtree only)
These methods use `Processor::Iterator<T> it(owner)` to search only the subtree rooted at the parent synth:

| Method | Iterator Type | Constraint |
|--------|--------------|------------|
| `getModulator` | `Modulator` | onInit only |
| `getMidiProcessor` | `MidiProcessor` | onInit only, self-exclusion |
| `getChildSynth` | `ModulatorSynth` | onInit only |
| `getChildSynthByIndex` | `Chain` handler | onInit only |
| `getEffect` | `EffectProcessor` | onInit only |
| `getAllEffects` | `EffectProcessor` | onInit only |
| `getAudioSampleProcessor` | `ProcessorWithExternalData` | requires AudioFile data |
| `getTableProcessor` | `ExternalDataHolder` | onInit only |
| `getSliderPackProcessor` | `ExternalDataHolder` | onInit only |
| `getDisplayBufferSource` | `ProcessorWithExternalData` | requires DisplayBuffer |
| `getSampler` | `ModulatorSampler` | onInit only |
| `getSlotFX` | `HotswappableProcessor` + `DspNetwork::Holder` | onInit only |
| `getIdList` | `Processor` (all types) | onInit only |

#### 2. Global-rooted search (entire module tree)
These methods use `ProcessorHelpers::getFirstProcessorWithName(getMainSynthChain(), ...)` or `Processor::Iterator<T>(getMainSynthChain())`:

| Method | Search Pattern | Notes |
|--------|---------------|-------|
| `getMidiPlayer` | `ProcessorHelpers::getFirstProcessorWithName` from main chain | No onInit restriction noted |
| `getRoutingMatrix` | `ProcessorHelpers::getFirstProcessorWithName` from main chain | No onInit restriction noted |
| `getWavetableController` | `ProcessorHelpers::getFirstProcessorWithName` from main chain | No onInit restriction noted |
| `getAllModulators` | `Processor::Iterator<Modulator>(getMainSynthChain())` | Searches entire tree |

#### 3. Backend diagnostic search
The `ModuleDiagnoser` template (backend only) uses `Processor::Iterator<T>(mc->getMainSynthChain())` to validate module names at compile time. This means the diagnostics search the entire tree even when the runtime method only searches the owner subtree.

### onInit-only Restriction Pattern

Most `get*()` methods check `getScriptProcessor()->objectsCanBeCreated()`, which returns `true` only during the `onInit` callback. If called elsewhere:
```cpp
reportIllegalCall("getModulator()", "onInit");
```

This is enforced because creating scripting wrapper objects (`new ScriptingObjects::Scripting*`) allocates memory and should not happen on the audio thread.

### Self-Exclusion in getMidiProcessor

```cpp
if(name == getProcessor()->getId())
{
    reportScriptError("You can't get a reference to yourself!");
}
```
This prevents the script processor from getting a handle to itself.

### getSlotFX Dual Search

`getSlotFX` has a unique dual-search pattern -- it first searches for `HotswappableProcessor` (the traditional SlotFX), then falls back to `DspNetwork::Holder` (ScriptNode-based slot):
```cpp
Processor::Iterator<HotswappableProcessor> it(owner);
// ... if not found ...
Processor::Iterator<DspNetwork::Holder> it2(owner);
```

---

## MIDI Event Generation

### Note Generation

Three ways to create notes:

1. **`playNote(noteNumber, velocity)`** -- Calls `internalAddNoteOn(1, noteNumber, velocity, 0, 0)`. Channel 1 forced, no start offset.

2. **`playNoteWithStartOffset(channel, number, velocity, offset)`** -- Calls `internalAddNoteOn(channel, number, velocity, 0, offset)`. Full control.

3. **`addNoteOn(channel, noteNumber, velocity, timeStampSamples)`** -- Calls `internalAddNoteOn(channel, noteNumber, velocity, timeStampSamples, 0)`. Explicit timestamp.

**`internalAddNoteOn` implementation details:**
- Creates `HiseEvent(HiseEvent::Type::NoteOn, noteNumber, velocity, channel)`
- Sets `m.setArtificial()` -- all script-generated notes are artificial
- If `HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS` (default on): subtracts one block from timestamps on audio thread
- Timestamp is relative to current event: `ce->getTimeStamp() + timeStampSamples`
- Start offset limited to `UINT16_MAX` (65536)
- Registers with EventHandler: `pushArtificialNoteOn(m)`
- Registers with Message object: `messageObject->pushArtificialNoteOn(m)`
- Returns: `m.getEventId()` (the artificial event ID)

**Validation:**
- Channel: 1-16
- Note number: 0-127
- Velocity: 0-127 (but playNote rejects 0)
- Timestamp: >= 0
- Start offset: <= 65536

### Note Off Methods

1. **`noteOff(noteNumber)`** -- DEPRECATED. Calls `addNoteOff(1, noteNumber, 0)` then reports error.

2. **`noteOffByEventId(eventId)`** -- Calls `noteOffDelayedByEventId(eventId, 0)`.

3. **`noteOffDelayedByEventId(eventId, timestamp)`** -- The canonical note-off method:
   - Pops note-on from EventHandler: `popNoteOnFromEventId(eventId)`
   - Safety check: refuses to kill non-artificial events
   - `HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS` adjustment applies
   - Timestamp relative to current event
   - If note already killed, calls `setArtificialTimestamp` to adjust timestamp

4. **`addNoteOff(channel, noteNumber, timeStampSamples)`** -- Low-level. Creates artificial NoteOff, minimum timestamp clamped to 1 (`jmax<int>(1, timeStampSamples)`). Finds matching event ID via `getEventIdForNoteOff`.

### Note Attachment System

**`setFixNoteOnAfterNoteOff(shouldBeFixed)`** -- Enables the attached note buffer in the MIDI processor chain. Must be called before `attachNote()`.

**`attachNote(originalNoteId, artificialNoteId)`** -- Links an artificial note to a real note so that when the real note is stopped, the artificial one is automatically stopped too. Requires `setFixNoteOnAfterNoteOff(true)` first.

### playNoteFromUI / noteOffFromUI

These inject MIDI via the keyboard state (message thread safe):
```cpp
CustomKeyboardState& state = getScriptProcessor()->getMainController_()->getKeyboardState();
state.injectMessage(MidiMessage::noteOn(channel, noteNumber, (uint8)velocity));
```
These simulate virtual keyboard input rather than inserting into the MIDI processor buffer.

### addMessageFromHolder

Inserts an event from a `ScriptingMessageHolder` object:
- Sets artificial flag
- For note-on: pushes to EventHandler, Message object, and MIDI buffer; returns event ID
- For note-off: assigns event ID from EventHandler, adds to buffer; returns timestamp
- For other events (CC, etc.): just adds to buffer; returns 0

---

## Voice Control

### Volume and Pitch Fades

**`addVolumeFade(eventId, fadeTimeMilliseconds, targetVolume)`**:
- Creates `HiseEvent::createVolumeFade(eventId, fadeTimeMs, targetVolume)`
- Inherits timestamp from current event
- **Special case: `targetVolume == -100`** -- This is "fade to silence and kill". After creating the volume fade, it also:
  - Pops the note-on from EventHandler
  - Creates a note-off with timestamp = fadeTimeMs converted to samples + 1
  - Only kills artificial events

**`addPitchFade(eventId, fadeTimeMilliseconds, targetCoarsePitch, targetFinePitch)`**:
- Creates `HiseEvent::createPitchFade(eventId, fadeTimeMs, coarse, fine)`
- Simpler than volume fade -- no auto-kill behavior

### Direct Voice Control

**`setVoiceGainValue(voiceIndex, gainValue)`** -- Calls `owner->setScriptGainValue(voiceIndex, gainValue)`.

**`setVoicePitchValue(voiceIndex, pitchValue)`** -- Calls `owner->setScriptPitchValue(voiceIndex, pitchValue)`. Range is 0.5 to 2.0 (pitch ratio).

These operate on voice indices (not event IDs), meaning they're for use in voice-start modulators or similar voice-indexed contexts.

---

## Controller Events

**`sendController(number, value)`**:
- Handles three cases:
  - `number == HiseEvent::PitchWheelCCNumber` (128): Creates PitchBend event, value range 0-16383
  - `number == HiseEvent::AfterTouchCCNumber` (129): Creates Aftertouch event
  - Normal CC: value range 0-127
- Timestamp inherited from current event
- **No channel parameter** -- uses default channel

**`addController(channel, number, value, timeStampSamples)`**:
- Same three-case handling as sendController
- Has explicit channel and timestamp
- Sets `e.setArtificial()` (unlike sendController which does NOT)
- Timestamp = currentEvent.timestamp + timeStampSamples

**`sendControllerToChildSynths(controllerNumber, controllerValue)`** -- Simply forwards to `sendController`. Exists for backwards compatibility.

---

## Timer System

### Architecture

The timer system has **two modes** depending on whether callbacks are deferred:

#### Non-deferred (audio thread) mode:
- Uses `ModulatorSynth::startSynthTimer(index, interval, timestamp)`
- There are exactly **4 timer slots** per ModulatorSynth (`synthTimerIntervals[0..3]`)
- Timer events are sample-accurate, rastered to `HISE_EVENT_RASTER`
- Timer fires by inserting `HiseEvent::createTimerEvent(index, offset)` into the event buffer
- Index is assigned per-script-processor via `parentMidiProcessor->getIndexInChain()`

#### Deferred (message thread) mode:
- Uses standard JUCE `Timer::startTimer((int)(intervalInSeconds * 1000))`
- No sample-accurate timing
- No slot limit

### Minimum interval
```cpp
if(intervalInSeconds < 0.004) // 4ms minimum
{
    reportScriptError("Go easy on the timer!");
    return;
}
```

### Timer methods:
- `startTimer(seconds)` -- Starts with the given interval
- `stopTimer()` -- Stops the timer, releases the slot
- `isTimerRunning()` -- Checks if active
- `getTimerInterval()` -- Returns interval in seconds

---

## Attribute System

**`setAttribute(attributeIndex, newAttribute)`** -- Sets an attribute on the `owner` synth:
```cpp
owner->setAttribute(attributeIndex, newAttribute, sendNotification);
```

**`getAttribute(attributeIndex)`** -- Gets an attribute:
```cpp
return owner->getAttribute(attributeIndex);
```

The attribute indices come from the owner's `Parameters` enum. For a standard `ModulatorSynth`:
- 0 = Gain (0.0-1.0)
- 1 = Balance (-100 to 100)
- 2 = VoiceLimit
- 3 = KillFadeTime

Subclasses (e.g., `ModulatorSampler`) extend this enum with additional parameters. The indices are processor-type-specific.

---

## Modulator Chain Access

**`addModulator(chainId, type, id)`**:
- chainId mapping: `0 = PitchModulation`, `1 = GainModulation` (from `ModulatorSynth::InternalChains`)
- Uses `moduleHandler.addModule(chain, type, id, -1)` (-1 = append)
- Returns a ScriptModulator handle

**`removeModulator(mod)`**:
- Takes a var, dynamic_casts to ScriptingModulator
- Uses `moduleHandler.removeModule(modToRemove)`

**`getModulatorIndex(chainId, id)`**:
- Returns the index of a modulator within a specific chain
- Same chainId mapping as addModulator

**`setModulatorAttribute(chainId, modulatorIndex, attributeIndex, newValue)`**:
- Finds the modulator in the chain at the given index
- Special attribute indices:
  - `-12` = Intensity (raw Modulation::setIntensity)
  - `-13` = Bypassed (setBypassed with `newValue == 1.0f`)
  - Other values: standard `setAttribute`
- For pitch chain, intensity is converted: `powf(2, newValue/12.0f)` (semitones to ratio)

---

## Effect Chain Access

**`addEffect(type, id, index)`**:
- Always adds to `owner->effectChain`
- `index = -1` to append

**`removeEffect(effect)`**:
- Takes a var, dynamic_casts to ScriptingEffect
- Uses moduleHandler.removeModule

---

## Clock Speed

**`setClockSpeed(clockSpeed)`**:
```cpp
switch (clockSpeed)
{
    case 0:  owner->setClockSpeed(ModulatorSynth::Inactive); break;  // 0xFFF
    case 1:  owner->setClockSpeed(ModulatorSynth::ClockSpeed::Bar); break;    // -2
    case 2:  owner->setClockSpeed(ModulatorSynth::ClockSpeed::Half); break;   // -1
    case 4:  owner->setClockSpeed(ModulatorSynth::ClockSpeed::Quarters); break; // 0
    case 8:  owner->setClockSpeed(ModulatorSynth::ClockSpeed::Eighths); break;  // 1
    case 16: owner->setClockSpeed(ModulatorSynth::ClockSpeed::Sixteens); break; // 2
    case 32: owner->setClockSpeed(ModulatorSynth::ThirtyTwos); break;          // 3
    default: reportScriptError("Unknown clockspeed. Use 1,2,4,8,16 or 32");
}
```

Valid values: 0 (inactive), 1, 2, 4, 8, 16, 32. The internal enum values are different from the script-facing values.

---

## Keyboard / Voice State

**`handleNoteCounter(const HiseEvent& e)`** -- Inline method called by the script processor on every event:
- Ignores artificial events
- NoteOn: increments `numPressedKeys`, sets bit in `keyDown`
- NoteOff: decrements `numPressedKeys` (clamped to 0), clears bit
- AllNotesOff: resets both to 0

**`getNumPressedKeys()`** -- Returns `numPressedKeys.get()`

**`isLegatoInterval()`** -- Returns `numPressedKeys.get() != 1` (true when 0 or 2+ keys pressed)

**`isKeyDown(noteNumber)`** -- Returns `keyDown[noteNumber]`

**`isSustainPedalDown()`** -- Returns `sustainState` (set externally via `setSustainPedal()`)

**`isArtificialEventActive(eventId)`** -- Checks with EventHandler:
```cpp
return getScriptProcessor()->getMainController_()->getEventHandler().isArtificialEventId((uint16)eventId);
```

---

## Other Methods

**`getNumChildSynths()`** -- Only works on Chain-type synths (SynthChain, SynthGroup):
```cpp
if(dynamic_cast<Chain*>(owner) == nullptr)
    reportScriptError("getNumChildSynths() can only be called on Chains!");
return dynamic_cast<Chain*>(owner)->getHandler()->getNumProcessors();
```

**`addToFront(addToFront)`** -- Delegates to `JavascriptMidiProcessor::addToFront()`. Marks this script's interface as the "front" interface.

**`deferCallbacks(makeAsynchronous)`** -- Delegates to `JavascriptMidiProcessor::deferCallbacks()`. When deferred:
- MIDI callbacks execute on message thread (not audio thread)
- MIDI messages become read-only
- Timer switches to JUCE Timer (not sample-accurate)

**`setMacroControl(macroIndex, newValue)`** -- Only works on `ModulatorSynthChain`:
```cpp
if(ModulatorSynthChain *chain = dynamic_cast<ModulatorSynthChain*>(owner))
    chain->setMacroControl(macroIndex - 1, newValue, sendNotification);
```
Index is 1-8 (one-based), value range 0.0-127.0.

**`setShouldKillRetriggeredNote(killNote)`** -- Sets whether retriggered notes are killed:
```cpp
owner->setKillRetriggeredNote(killNote);
```

**`setUseUniformVoiceHandler(containerId, shouldUse)`** -- **DEPRECATED**:
```cpp
reportScriptError("This function is deprecated. Just remove that call and enjoy global envelopes...");
```

**`createBuilder()`** -- Creates a `ScriptBuilder` object:
```cpp
return var(new ScriptingObjects::ScriptBuilder(getScriptProcessor()));
```

---

## ModuleHandler (Helper Class)

`ApiHelpers::ModuleHandler` is used by `addModulator`, `removeModulator`, `addEffect`, and `removeEffect`:

```cpp
class ModuleHandler
{
public:
    ModuleHandler(Processor* parent_, JavascriptProcessor* sp);
    ~ModuleHandler();
    bool removeModule(Processor* p);
    Processor* addModule(Chain* c, const String& type, const String& id, int index = -1);
    Modulator* addAndConnectToGlobalModulator(Chain* c, Modulator* globalModulator, 
                                               const String& modName, bool connectAsStaticMod = false);
    JavascriptProcessor* getScriptProcessor();
private:
    WeakReference<Processor> parent;
    WeakReference<JavascriptProcessor> scriptProcessor;
    Component::SafePointer<Component> mainEditor;
};
```

This is the mechanism that Builder also uses internally for module creation/removal. The `addModule` method handles the thread-safe creation pattern using `KillStateHandler`.

---

## ModuleDiagnoser (Backend Diagnostic System)

```cpp
#if USE_BACKEND
struct ModuleDiagnoser
{
    template <typename T> static ApiClass::DiagnosticResult check(ApiClass* c, const Identifier&, const Array<var>& args)
    {
        // Searches entire tree from getMainSynthChain() for module name
        // Returns ok() if found, fail() with fuzzy suggestion if not
    }
};
#endif
```

**Diagnostic macros in constructor:**
```cpp
#define CHECK_MODULE(methodName, className) addDiagnostic(#methodName, ModuleDiagnoser::check<className>)
```

**Methods with diagnostics and their search types:**

| Method | C++ Type Searched |
|--------|------------------|
| getMidiPlayer | `hise::MidiPlayer` |
| getModulator | `Modulator` |
| getAudioSampleProcessor | `ProcessorWithExternalData` |
| getDisplayBufferSource | `ProcessorWithExternalData` |
| getTableProcessor | `ProcessorWithExternalData` |
| getSliderPackProcessor | `ProcessorWithExternalData` |
| getWavetableController | `hise::WavetableSynth` |
| getSampler | `ModulatorSampler` |
| getSlotFX | `HotswappableProcessor` |
| getEffect | `EffectProcessor` |
| getRoutingMatrix | `hise::RoutableProcessor` |
| getMidiProcessor | `hise::MidiProcessor` |
| getChildSynth | `hise::ModulatorSynth` |

The diagnostic result includes a fuzzy-suggestion system: if the module name is not found, it collects all IDs of the searched type and suggests the closest match.

---

## ModuleIds Class

`ScriptingApi::ModuleIds` is a separate ApiClass that's registered alongside Synth. It generates string constants for all module types available in the owner synth's factory chains:

```cpp
ModuleIds::ModuleIds(ModulatorSynth* s):
    ApiClass(getTypeList(s).size()),
    ownerSynth(s)
{
    auto list = getTypeList(ownerSynth);
    list.sort();
    for (int i = 0; i < list.size(); i++)
        addConstant(list[i].toString(), list[i].toString());
}
```

`getTypeList` iterates all internal chains of the owner synth and collects the `FactoryType::getAllowedTypes()` from each chain. The constants are the processor type names (e.g., "SimpleEnvelope", "LFOModulator", etc.).

These are used as the `type` parameter for `addModulator()` and `addEffect()`.

---

## Preprocessor Guards

| Preprocessor | Effect |
|-------------|--------|
| `USE_BACKEND` | ModuleDiagnoser compile-time checks, CHECK_MODULE macro |
| `ENABLE_SCRIPTING_SAFE_CHECKS` | Extra validation (deprecated noteOff warning, artificial event check). Default: 1 |
| `HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS` | Subtracts one block from timestamps on audio thread. Default: 1 |
| `FRONTEND_ONLY(...)` | Code that runs only in exported plugins |
| `HISE_EVENT_RASTER` | Sample alignment grid for timer events |

---

## Host Processor Types

The Synth object is created in multiple script processor types. Each `registerApiClasses()` creates it with the same pattern:

```cpp
synthObject = new ScriptingApi::Synth(this, currentMidiMessage.get(), getOwnerSynth());
```

**Processors that create Synth objects:**

| Processor Class | Type | Has MIDI Callbacks |
|----------------|------|-------------------|
| `JavascriptMidiProcessor` | Script Processor | Yes (onNoteOn, onNoteOff, onController, onTimer, onControl) |
| `JavascriptVoiceStartModulator` | Script Voice Start Modulator | Yes (onVoiceStart, onVoiceStop, onController, onControl) |
| `JavascriptTimeVariantModulator` | Script Time Variant Modulator | Yes (onNoteOn, onNoteOff, onController, onControl) |
| `JavascriptEnvelopeModulator` | Script Envelope Modulator | Limited (onControl) |
| `JavascriptMasterEffect` | Script FX | Yes (has MIDI callbacks) |
| `JavascriptPolyphonicEffect` | Script Polyphonic FX | Yes |
| `JavascriptSynthesiser` | Script Synthesiser | Yes |
| `HardcodedScriptProcessor` | C++ hardcoded scripts | Yes |

**Important:** The `parentMidiProcessor` cast succeeds only for `ScriptBaseMidiProcessor` subclasses. For modulators and effects, it will be null, which means MIDI-generating methods (`playNote`, `addNoteOn`, `addVolumeFade`, etc.) will fail with "Only valid in MidiProcessors".

---

## Threading Model

**Audio thread methods (require parentMidiProcessor):**
- All note generation: `playNote`, `addNoteOn`, `addNoteOff`, etc.
- Volume/pitch fades: `addVolumeFade`, `addPitchFade`
- Controller sending: `sendController`, `addController`
- Timer: `startTimer`, `stopTimer` (non-deferred mode)
- Message holder: `addMessageFromHolder`

**onInit-only methods (checked via objectsCanBeCreated()):**
- All `get*()` methods (getModulator, getEffect, getMidiProcessor, etc.)
- Note: `getMidiPlayer`, `getRoutingMatrix`, `getWavetableController` do NOT check `objectsCanBeCreated()`

**Any-thread methods:**
- `getAttribute`, `setAttribute` (direct processor access)
- `getNumPressedKeys`, `isKeyDown`, `isLegatoInterval`, `isSustainPedalDown`
- `isArtificialEventActive`
- `getNumChildSynths`
- `setClockSpeed`, `setShouldKillRetriggeredNote`
- `setMacroControl` (checks for ModulatorSynthChain)

**Audio thread warnings (WARN_IF_AUDIO_THREAD):**
- `getEffect`, `getAllEffects`, `getAudioSampleProcessor`, `getTableProcessor`, `getSliderPackProcessor`, `getDisplayBufferSource`, `getSampler`, `getSlotFX`

---

## Synth Object Lifecycle

The Synth object's lifetime is tied to the script processor that creates it. It is stored as a raw pointer (`ScriptingApi::Synth *synthObject`) in the processor class. Key lifecycle events:

1. **Creation:** During `registerApiClasses()` in each script processor type
2. **Registration:** `scriptEngine->registerApiClass(synthObject)` -- makes it a global `Synth` namespace in scripts
3. **MIDI processing:** `synthObject->handleNoteCounter(*currentEvent)` is called on every MIDI event before callbacks
4. **Sustain tracking:** `synthObject->setSustainPedal(shouldBeDown)` is called externally

---

## ModulatorSynth::InternalChains Enum (Critical for chainId parameters)

From `ModulatorSynth.h`:
```cpp
enum InternalChains
{
    MidiProcessor = 0,
    GainModulation,    // = 1
    PitchModulation,   // = 2
    EffectChain,       // = 3
    numInternalChains
};
```

**But in the scripting API, the chainId mapping for addModulator/setModulatorAttribute is:**
- `0` = PitchModulation (confusingly, this maps to `ModulatorSynth::PitchModulation` = 2)
- `1` = GainModulation (maps to `ModulatorSynth::GainModulation` = 1)

Wait -- let me re-check. Looking at the code more carefully:

```cpp
switch(chain)
{
    case ModulatorSynth::GainModulation:  c = owner->gainChain; break;   // case 1
    case ModulatorSynth::PitchModulation: c = owner->pitchChain; break;  // case 2
    default: reportScriptError("No valid chainType - 1= GainModulation, 2=PitchModulation");
}
```

So the scripting chainId uses the **same values as the C++ enum**: `1 = GainModulation`, `2 = PitchModulation`. The error message in `setModulatorAttribute` says "GainModulation = 1, PitchModulation = 0" but the code uses the enum values which are `GainModulation = 1, PitchModulation = 2`. The doxygen description "GainModulation = 1, PitchModulation = 0" in the base JSON appears to be wrong.

Actually, looking at the `setModulatorAttribute` error message:
```
"No valid chainType - 1= GainModulation, 2=PitchModulation"
```

But the description in the JSON says:
```
"chainId the chain where the Modulator is. GainModulation = 1, PitchModulation = 0"
```

The **code** clearly uses `case ModulatorSynth::GainModulation` (=1) and `case ModulatorSynth::PitchModulation` (=2). The description `PitchModulation = 0` in the JSON is incorrect -- it should be 2. This is a documentation error.

---

## HiseEvent Special Constants

Used in `sendController` and `addController`:
```cpp
static constexpr int PitchWheelCCNumber = 128;
static constexpr int AfterTouchCCNumber = 129;
```

These allow pitch bend and aftertouch to be sent using the same controller-number interface.

---

## Backwards Compatibility Notes

1. **`noteOff(noteNumber)`** -- Deprecated. Still works but reports error. Use `noteOffByEventId` instead.
2. **`sendControllerToChildSynths`** -- Exists only for backwards compatibility, just calls `sendController`.
3. **`setUseUniformVoiceHandler`** -- Fully deprecated, throws error immediately.
4. **`HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS`** -- Default on. Adjusts timestamp behavior for old patches.
