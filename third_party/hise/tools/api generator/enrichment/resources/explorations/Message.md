# Message -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- enrichment prerequisites
- `enrichment/resources/survey/class_survey_data.json` -- Message entry (createdBy, seeAlso)
- `enrichment/base/Message.json` -- authoritative method list (36 methods)
- No prerequisite class required per the survey table (Message has no "Enrich First" dependency)

## Source Files

| File | Role |
|------|------|
| `hi_scripting/scripting/api/ScriptingApi.h` lines 58-231 | Class declaration |
| `hi_scripting/scripting/api/ScriptingApi.cpp` lines 400-1150 | Full implementation |
| `hi_tools/hi_tools/HiseEventBuffer.h` lines 47-476 | HiseEvent class declaration |
| `hi_tools/hi_tools/HiseEventBuffer.h` lines 644-716 | EventIdHandler class |
| `hi_tools/hi_tools/HiseEventBuffer.cpp` lines 35-370 | HiseEvent implementation |
| `hi_tools/hi_tools/HiseEventBuffer.cpp` lines 937-1122 | EventIdHandler implementation |
| `hi_scripting/scripting/ScriptProcessorModules.h` lines 62-192 | JavascriptMidiProcessor |
| `hi_scripting/scripting/ScriptProcessorModules.cpp` lines 120-465 | Callback dispatch |
| `hi_scripting/scripting/ScriptProcessor.h` lines 217-264 | ScriptBaseMidiProcessor |
| `hi_scripting/scripting/api/ScriptingApiObjects.h` lines 1478-1620 | ScriptingMessageHolder |
| `hi_scripting/scripting/api/ScriptingApi.cpp` lines 6353-6509 | Synth.addNoteOn/addNoteOff/addController |

---

## Class Declaration

```cpp
class ScriptingApi::Message : public ScriptingObject, public ApiClass
```

**Location:** `hi_scripting/scripting/api/ScriptingApi.h` line 58

The Message class is a nested class inside `ScriptingApi`. It inherits from:
- `ScriptingObject` -- provides `reportScriptError()`, `reportIllegalCall()`, `getScriptProcessor()` (links back to the processor context)
- `ApiClass` -- provides the HISE scripting API registration infrastructure (`addConstant()`, `ADD_API_METHOD_N` macros, method dispatch)

It is NOT a `ConstScriptingObject` -- it does not have a `numArgs` constructor or the typical `addScriptParameters()` pattern. Instead it acts as a namespace-like singleton attached to each script processor, registered via `scriptEngine->registerApiClass(currentMidiMessage.get())`.

### Key Private Members

```cpp
HiseEvent* messageHolder;             // Mutable pointer -- set for callbacks that allow modification
const HiseEvent* constMessageHolder;  // Const pointer -- always set when message is active
uint16 artificialNoteOnIds[128];      // Local cache of artificial note-on event IDs per note number
HiseEvent artificialNoteOnThatWasKilled; // Preserved note-on for ignoreEvent reinsert logic
WeakCallbackHolder allNotesOffCallback;  // Callback for all-notes-off
```

### Friend Classes

```cpp
friend class Synth;
friend class JavascriptMidiProcessor;
friend class HardcodedScriptProcessor;
```

---

## HiseEvent -- The 16-Byte Packed Event Format

**Location:** `hi_tools/hi_tools/HiseEventBuffer.h` lines 83-476

HiseEvent is a fixed-size 128-bit (16-byte) struct that replaces JUCE's `MidiMessage` for all internal HISE event processing. The fixed size enables trivial memcpy/memset operations on event buffers.

### Memory Layout

```
DWord 1 (bytes 0-3):
  Type type       : uint8   -- event type enum
  uint8 channel   : uint8   -- MIDI channel (1-based, supports up to 256 in HISE vs 16 in MIDI)
  uint8 number    : uint8   -- note number, CC number, or general data byte 1
  uint8 value     : uint8   -- velocity, CC value, or general data byte 2

DWord 2 (bytes 4-7):
  int8 transposeValue  -- transpose amount in semitones
  int8 gain            -- gain in decibels (-100 to 36, clamped)
  int8 semitones       -- coarse detune in semitones
  int8 cents           -- fine detune in cents

DWord 3 (bytes 8-11):
  uint16 eventId       -- unique event identifier
  uint16 startOffset   -- sample offset for voice start

DWord 4 (bytes 12-15):
  uint32 timestamp     -- sample timestamp + flags in upper 2 bits
```

### Timestamp Bit Packing

The `timestamp` field packs three pieces of information into 32 bits:

| Bits | Mask | Meaning |
|------|------|---------|
| 31 (MSB) | `0x80000000` | Artificial flag -- set if event was created by script/engine |
| 30 | `0x40000000` | Ignored flag -- set by `ignoreEvent(true)` |
| 0-29 | `0x3FFFFFFF` | Actual timestamp (sample offset, max ~1 billion samples) |

The `getTimeStamp()` method masks with `0x0FFFFFFF` (note: slightly smaller mask than the full 30 bits, leaving bit 28-29 for potential future use), while `setTimeStamp()` preserves the top 2 flag bits and writes the lower 30.

### Event Types

```cpp
enum class Type : uint8 {
    Empty = 0,       // Default-constructed empty event
    NoteOn,          // Note-on (gets unique EventID)
    NoteOff,         // Note-off (matched to corresponding note-on EventID)
    Controller,      // MIDI CC message
    PitchBend,       // 14-bit pitch bend
    Aftertouch,      // Both channel pressure and poly aftertouch
    AllNotesOff,     // All notes off
    SongPosition,    // DAW transport position
    MidiStart,       // DAW playback start
    MidiStop,        // DAW playback stop
    VolumeFade,      // Internal: volume fade targeting an EventID
    PitchFade,       // Internal: pitch fade targeting an EventID
    TimerEvent,      // Internal: fires onTimer callback
    ProgramChange,   // MIDI program change
    numTypes
};
```

### Controller Number Conventions

The `getControllerNumber()` method in HiseEvent returns virtual CC numbers for non-CC types:

```cpp
int HiseEvent::getControllerNumber() const noexcept {
    if (type == Type::PitchBend)   return PitchWheelCCNumber; // 128
    if (type == Type::Aftertouch)  return AfterTouchCCNumber; // 129
    return number;
}
```

Static constants:
- `HiseEvent::PitchWheelCCNumber = 128`
- `HiseEvent::AfterTouchCCNumber = 129`

### Pitch Wheel Value Encoding

Pitch wheel uses both `number` and `value` bytes to store a 14-bit value:
```cpp
int getPitchWheelValue() const noexcept { return number | (value << 7); }
void setPitchWheelValue(int position) noexcept {
    number = position & 127;
    value = (position >> 7) & 127;
}
```

### Gain Field

The gain field is clamped to -100..36 dB:
```cpp
void setGain(int decibels) noexcept { gain = (int8)jlimit<int>(-100, 36, decibels); }
```

### Start Offset

The `startOffset` is a `uint16` (max 65535) that tells the sound generator to skip samples at voice start. This is stored separately from the timestamp -- timestamp delays the event's processing, while startOffset changes where in the sample playback begins.

---

## Constructor -- Constants and Method Registration

**Location:** `hi_scripting/scripting/api/ScriptingApi.cpp` lines 447-518

```cpp
ScriptingApi::Message::Message(ProcessorWithScriptingContent *p) :
    ScriptingObject(p),
    ApiClass(11),  // 11 = number of constants
    messageHolder(nullptr),
    constMessageHolder(nullptr),
    allNotesOffCallback(p, nullptr, var(), 0)
```

### Constants (addConstant calls)

| Name | Value | Source |
|------|-------|--------|
| `PITCH_BEND_CC` | 128 | `HiseEvent::PitchWheelCCNumber` |
| `AFTERTOUC_CC` | 129 | `HiseEvent::AfterTouchCCNumber` (note: typo in name, missing 'H') |
| `Empty ` | 0 | `HiseEvent::Type::Empty` (note: trailing space in name!) |
| `NoteOn` | 1 | `HiseEvent::Type::NoteOn` |
| `NoteOff` | 2 | `HiseEvent::Type::NoteOff` |
| `Controller` | 3 | `HiseEvent::Type::Controller` |
| `PitchBend` | 4 | `HiseEvent::Type::PitchBend` |
| `Aftertouch` | 5 | `HiseEvent::Type::Aftertouch` |
| `AllNotesOff` | 6 | `HiseEvent::Type::AllNotesOff` |
| `VolumeFade` | 10 | `HiseEvent::Type::VolumeFade` |
| `PitchFade` | 11 | `HiseEvent::Type::PitchFade` |

Note: The enum values 7-9 (SongPosition, MidiStart, MidiStop) and 12 (TimerEvent) and 13 (ProgramChange) are NOT exposed as constants.

### Typed Method Registrations (ADD_TYPED_API_METHOD_1)

All single-parameter setter methods use typed registration with explicit VarTypeChecker:

| Method | Type | Registration |
|--------|------|-------------|
| `setNoteNumber` | Number | `ADD_TYPED_API_METHOD_1(setNoteNumber, VarTypeChecker::Number)` |
| `setVelocity` | Number | `ADD_TYPED_API_METHOD_1(setVelocity, VarTypeChecker::Number)` |
| `setControllerNumber` | Number | `ADD_TYPED_API_METHOD_1(setControllerNumber, VarTypeChecker::Number)` |
| `setControllerValue` | Number | `ADD_TYPED_API_METHOD_1(setControllerValue, VarTypeChecker::Number)` |
| `delayEvent` | Number | `ADD_TYPED_API_METHOD_1(delayEvent, VarTypeChecker::Number)` |
| `setChannel` | Number | `ADD_TYPED_API_METHOD_1(setChannel, VarTypeChecker::Number)` |
| `setGain` | Number | `ADD_TYPED_API_METHOD_1(setGain, VarTypeChecker::Number)` |
| `setTransposeAmount` | Number | `ADD_TYPED_API_METHOD_1(setTransposeAmount, VarTypeChecker::Number)` |
| `setCoarseDetune` | Number | `ADD_TYPED_API_METHOD_1(setCoarseDetune, VarTypeChecker::Number)` |
| `setFineDetune` | Number | `ADD_TYPED_API_METHOD_1(setFineDetune, VarTypeChecker::Number)` |
| `setStartOffset` | Number | `ADD_TYPED_API_METHOD_1(setStartOffset, VarTypeChecker::Number)` |
| `store` | ScriptObject | `ADD_TYPED_API_METHOD_1(store, VarTypeChecker::ScriptObject)` |
| `setAllNotesOffCallback` | Function | `ADD_TYPED_API_METHOD_1(setAllNotesOffCallback, VarTypeChecker::Function)` |

The `ignoreEvent` method uses `ADD_API_METHOD_1` (plain, not typed).
The `setMonophonicAfterTouchPressure` method also uses `ADD_API_METHOD_1`.
The `setPolyAfterTouchNoteNumberAndPressureValue` uses `ADD_API_METHOD_2`.

### Callback Diagnostic

```cpp
ADD_CALLBACK_DIAGNOSTIC(allNotesOffCallback, setAllNotesOffCallback, 0);
```

This registers a diagnostic for the `setAllNotesOffCallback` method: the callback takes 0 parameters.

### Wrapper Struct

All methods use plain `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` macros (no typed wrappers in the Wrapper struct itself). The wrappers provide the bridge between the scripting engine's dynamic dispatch and the typed C++ methods.

---

## Factory / obtainedVia

The Message object is NOT created by user code. It is a singleton per script processor instance, created internally during `registerApiClasses()`:

```cpp
// In JavascriptMidiProcessor::registerApiClasses():
currentMidiMessage = new ScriptingApi::Message(this);
scriptEngine->registerApiClass(currentMidiMessage.get());
```

The same pattern appears in `JavascriptVoiceStartModulator::registerApiClasses()` and other script processor types. The `Message` object is registered as an API class accessible via the global name `Message` in HiseScript.

---

## Threading Model -- How the Engine Sets Up the Message

### Audio Thread (Normal Path)

In `JavascriptMidiProcessor::processHiseEvent(HiseEvent &m)`:

```cpp
void JavascriptMidiProcessor::processHiseEvent(HiseEvent &m)
{
    if (isDeferred()) {
        deferredExecutioner.addPendingEvent(m);
    } else {
        ScopedValueSetter<HiseEvent*> svs(currentEvent, &m);
        currentMidiMessage->setHiseEvent(m);
        runScriptCallbacks();
    }
}
```

Key observations:
1. `currentEvent` (on ScriptBaseMidiProcessor) is set via ScopedValueSetter -- it's only valid during the callback
2. `setHiseEvent(HiseEvent &m)` sets BOTH `messageHolder` and `constMessageHolder` to point to the event
3. The event `m` is passed by reference from the HiseEventBuffer -- modifications are in-place
4. After the callback returns, `currentEvent` resets to its previous value (typically nullptr)

### Mutable vs Const Pointer Pattern

Two `setHiseEvent` overloads control write access:

```cpp
void Message::setHiseEvent(HiseEvent &m) {
    messageHolder = &m;           // Mutable access
    constMessageHolder = messageHolder;
}

void Message::setHiseEvent(const HiseEvent& m) {
    messageHolder = nullptr;       // No mutable access
    constMessageHolder = &m;
}
```

When `messageHolder` is nullptr, all setter methods report an error via `reportIllegalCall()`. This is how read-only contexts (like voice start modulators receiving `const HiseEvent&`) prevent scripts from modifying events they shouldn't.

### Deferred Mode (Message Thread)

When `isDeferred()` is true, events are queued to a `LockfreeQueue<HiseEvent>` and processed on the scripting thread via `JavascriptThreadPool::addJob()`. The deferred handler creates a copy of the event and sets the Message with the non-const reference to the copy, but crucially:

```cpp
// In DeferredExecutioner::handleAsyncUpdate():
if (m.isIgnored() || m.isArtificial())
    continue;  // Artificial/ignored events are SKIPPED in deferred mode
```

This means deferred scripts cannot see artificial events or already-ignored events.

### Voice Start Modulator Context

```cpp
void JavascriptVoiceStartModulator::handleHiseEvent(const HiseEvent& m) {
    currentMidiMessage->setHiseEvent(m);  // Const overload -- read-only!
    ...
}
```

Voice start modulators receive a const reference, so `messageHolder` is null and all setter methods will report errors.

---

## Safe Checks and Error Reporting

Most methods follow a consistent pattern using `ENABLE_SCRIPTING_SAFE_CHECKS`:

```cpp
#if ENABLE_SCRIPTING_SAFE_CHECKS
    if (constMessageHolder == nullptr || !constMessageHolder->isNoteOnOrOff()) {
        reportIllegalCall("getNoteNumber()", "onNoteOn / onNoteOff");
        RETURN_IF_NO_THROW(-1)
    }
#endif
```

### Error Check Categories

1. **Null pointer check** (`messageHolder == nullptr` or `constMessageHolder == nullptr`) -- catches calls outside callback scope
2. **Event type check** -- many methods verify the event type matches (e.g., `getNoteNumber()` requires `isNoteOnOrOff()`)
3. **Range validation** -- `setChannel()` checks 1-16, `setStartOffset()` checks against `UINT16_MAX`

### Specific Method Constraints (from source)

| Method | Required Event Type | Required Context |
|--------|-------------------|------------------|
| `getNoteNumber()` | NoteOn or NoteOff | Any MIDI callback |
| `setNoteNumber()` | NoteOn or NoteOff | Mutable callback |
| `getVelocity()` | NoteOn or NoteOff | Any MIDI callback |
| `setVelocity()` | NoteOn only | Mutable callback |
| `getControllerNumber()` | Controller, PitchWheel, or Aftertouch | onController |
| `getControllerValue()` | Controller, PitchWheel, or Aftertouch | onController |
| `setControllerNumber()` | Controller | Mutable callback |
| `setControllerValue()` | Controller | Mutable callback |
| `getMonophonicAftertouchPressure()` | ChannelPressure (Aftertouch) | MIDI callback |
| `setMonophonicAfterTouchPressure()` | ChannelPressure (Aftertouch) | Mutable callback |
| `getPolyAfterTouchNoteNumber()` | Aftertouch | MIDI callback |
| `getPolyAfterTouchPressureValue()` | Aftertouch | MIDI callback |
| `setPolyAfterTouchNoteNumberAndPressureValue()` | Aftertouch | Mutable callback |
| `isProgramChange()` | Any (returns bool) | MIDI callback |
| `getProgramChangeNumber()` | ProgramChange | MIDI callback |

Note: `getChannel()` only checks for null, not for any specific event type. Same for `getEventId()`, `getTimestamp()`, `getTransposeAmount()`, `getCoarseDetune()`, `getFineDetune()`, `getGain()`, `getStartOffset()`.

### FRONTEND_ONLY Macro

Some methods use `FRONTEND_ONLY(return 0;)` instead of `RETURN_IF_NO_THROW`:
- `getEventId()` -- returns 0 in frontend builds when called outside callback
- `setTransposeAmount()` / `getTransposeAmount()` -- use `FRONTEND_ONLY(return;)` / `FRONTEND_ONLY(return 0;)`

This means in frontend (exported plugin) builds, these methods silently return defaults instead of throwing errors when called outside callbacks. In backend builds, they will hit `reportIllegalCall()`.

---

## Callback Dispatch -- Which Events Trigger Which Callbacks

**Location:** `ScriptProcessorModules.cpp` lines 335-448

```cpp
switch (currentEvent->getType()) {
    case HiseEvent::Type::NoteOn:      -> onNoteOn callback
    case HiseEvent::Type::NoteOff:     -> onNoteOff callback
    case HiseEvent::Type::Controller:
    case HiseEvent::Type::PitchBend:
    case HiseEvent::Type::Aftertouch:
    case HiseEvent::Type::ProgramChange: -> onController callback
    case HiseEvent::Type::TimerEvent:  -> onTimer callback (if matching processor index)
    case HiseEvent::Type::AllNotesOff: -> triggers onAllNotesOff callback (via Message)
}
```

Key insight: **PitchBend, Aftertouch, and ProgramChange all trigger the onController callback.** This is why `getControllerNumber()` returns 128/129 for pitch wheel/aftertouch -- scripts in onController can handle all three event types uniformly using the virtual CC number convention.

### AllNotesOff Handling

AllNotesOff events are handled specially -- they call `currentMidiMessage->onAllNotesOff()` which synchronously invokes the callback registered via `setAllNotesOffCallback()`. They do NOT trigger onController or any other standard callback.

### Sustain Pedal Special Case

CC64 (sustain pedal) is handled specially before the onController callback:
```cpp
if (currentEvent->isControllerOfType(64)) {
    synthObject->setSustainPedal(currentEvent->getControllerValue() > 64);
}
```

---

## The Artificial Event System

### What Makes an Event "Artificial"

An event is artificial if:
1. It was created by `Message.makeArtificial()` or `Message.makeArtificialOrLocal()`
2. It was created by `Synth.addNoteOn()`, `Synth.addNoteOff()`, or `Synth.addController()`
3. It was created by internal HISE mechanisms (VolumeFade, PitchFade, TimerEvent factory methods)

The artificial flag is stored in bit 31 of the timestamp field.

### makeArtificial() vs makeArtificialOrLocal()

Both call `makeArtificialInternal(bool makeLocal)`:

```cpp
int Message::makeArtificialInternal(bool makeLocal) {
    artificialNoteOnThatWasKilled = {};  // Reset preserved note-on
    
    if (messageHolder != nullptr) {
        HiseEvent copy(*messageHolder);
        
        // makeArtificial: if already artificial, return existing ID
        // makeArtificialOrLocal: always create new artificial event
        if (!makeLocal && copy.isArtificial())
            return copy.getEventId();
        
        copy.setArtificial();
        
        if (copy.isNoteOn()) {
            // Push to EventIdHandler (gets new ID) and local cache
            getScriptProcessor()->getMainController_()->getEventHandler().pushArtificialNoteOn(copy);
            pushArtificialNoteOn(copy);
        }
        else if (copy.isNoteOff()) {
            // Pop matching note-on from local cache
            HiseEvent e = getScriptProcessor()->getMainController_()->getEventHandler()
                .popNoteOnFromEventId(artificialNoteOnIds[copy.getNoteNumber()]);
            
            artificialNoteOnThatWasKilled = e;  // Preserve for potential ignoreEvent reinsert
            
            if (e.isEmpty()) {
                // No matching note-on found -- ignore this note-off
                artificialNoteOnIds[copy.getNoteNumber()] = 0;
                copy.ignoreEvent(true);
            }
            
            copy.setEventId(artificialNoteOnIds[copy.getNoteNumber()]);
        }
        
        copy.swapWith(*messageHolder);  // Replace the original event in the buffer
        return messageHolder->getEventId();
    }
    return 0;
}
```

### The Difference

- `makeArtificial()`: Idempotent -- if the event is already artificial, it returns the existing event ID without creating a duplicate. This is the common case for "I want to modify this event."
- `makeArtificialOrLocal()`: Always creates a new artificial event with a new ID, even if the event is already artificial. This is useful for creating multiple copies/branches of an event.

### artificialNoteOnIds Array

```cpp
uint16 artificialNoteOnIds[128];
```

This is a per-Message-instance (i.e., per script processor) cache mapping note numbers to their most recent artificial event IDs. This is necessary because `makeArtificial()` on a note-off needs to find the matching note-on's artificial event ID.

### pushArtificialNoteOn (local)

```cpp
void pushArtificialNoteOn(const HiseEvent& e) {
    jassert(e.isArtificial());
    artificialNoteOnIds[e.getNoteNumber()] = e.getEventId();
}
```

This caches the event ID locally per note number. It's separate from the global `EventIdHandler::pushArtificialNoteOn()`.

---

## EventIdHandler -- Global Event ID Management

**Location:** `hi_tools/hi_tools/HiseEventBuffer.h` lines 644-716, implementation in HiseEventBuffer.cpp

### Architecture

The EventIdHandler is owned by the MainController and operates on the master HiseEventBuffer. It assigns sequential event IDs to note-on/note-off pairs before any script processing occurs.

### Data Structures

```cpp
const HiseEventBuffer &masterBuffer;
HeapBlock<HiseEvent> artificialEvents;           // Ring buffer, size HISE_EVENT_ID_ARRAY_SIZE (16384)
uint16 lastArtificialEventIds[16][128];           // [channel][noteNumber] -> last artificial event ID
HiseEvent realNoteOnEvents[16][128];              // [channel][noteNumber] -> real note-on event
uint16 currentEventId;                            // Monotonically increasing counter (wraps at 65536)
UnorderedStack<HiseEvent, 256> overlappingNoteOns; // Handles overlapping notes on same note number
```

### handleEventIds() -- Real Event ID Assignment

Called once per buffer before script processing. For each event in the master buffer:

1. **NoteOn**: Assigns `currentEventId++` to the event. Stores in `realNoteOnEvents[channel][noteNumber]`. If that slot is already occupied (overlapping note), pushes existing to `overlappingNoteOns`.

2. **NoteOff**: Finds matching note-on by channel + note number. First checks `realNoteOnEvents`, then `overlappingNoteOns`. Copies the note-on's event ID to the note-off. Also copies the transpose amount so note-off matches note-on's transposition. If no match found, assigns a new ID and ignores the event.

3. **AllNotesOff**: Moves all active note-ons from `realNoteOnEvents` into `overlappingNoteOns` (so subsequent note-offs can still find their matches).

### pushArtificialNoteOn (global)

```cpp
void EventIdHandler::pushArtificialNoteOn(HiseEvent& noteOnEvent) noexcept {
    noteOnEvent.setEventId(currentEventId);
    artificialEvents[currentEventId % HISE_EVENT_ID_ARRAY_SIZE] = noteOnEvent;
    lastArtificialEventIds[noteOnEvent.getChannel() % 16][noteOnEvent.getNoteNumber()] = currentEventId;
    currentEventId++;
}
```

This assigns the next event ID to the artificial note-on and stores it in the ring buffer. The ring buffer has 16384 slots, so event IDs wrap around.

### Event ID Wrapping

Event IDs are `uint16` (0-65535). They wrap around at 65536. The comment in `HiseEvent::getEventId()` notes:
> "Be aware the event ID is stored as unsigned 16 bit integer, so it will wrap around 65536. It's highly unlikely that you will hit any collisions, but you can't expect that older notes have a higher event ID."

---

## ignoreEvent -- Pipeline Mechanics

```cpp
void Message::ignoreEvent(bool shouldBeIgnored) {
    if (messageHolder == nullptr) {
        reportIllegalCall("ignoreEvent()", "midi event");
        RETURN_VOID_IF_NO_THROW()
    }
    
    // Special case: if ignoring an artificial note-off, reinsert the matching note-on
    if (shouldBeIgnored && isArtificial() && messageHolder->isNoteOff() && 
        (artificialNoteOnThatWasKilled.getEventId() == messageHolder->getEventId())) 
    {
        getScriptProcessor()->getMainController_()->getEventHandler()
            .reinsertArtificialNoteOn(artificialNoteOnThatWasKilled);
        pushArtificialNoteOn(artificialNoteOnThatWasKilled);
    }
    
    messageHolder->ignoreEvent(shouldBeIgnored);
}
```

### What "Ignored" Means at the Engine Level

Setting the ignored flag (bit 30 of timestamp) causes:
- The event remains in the buffer but is skipped by downstream processors
- The `HiseEventBuffer::Iterator` can optionally skip ignored events (`skipIgnoredEvents` parameter)
- In deferred mode, ignored events are skipped entirely: `if (m.isIgnored() || m.isArtificial()) continue;`

### Note-Off Reinsert Logic

The special reinsert logic handles this scenario:
1. Script calls `makeArtificial()` on a note-on (which pops the original note-on from EventIdHandler and stores it in `artificialNoteOnThatWasKilled`)
2. The corresponding note-off arrives
3. Script calls `makeArtificial()` on the note-off (which pops the artificial note-on from EventIdHandler)
4. Script then calls `ignoreEvent(true)` on that note-off

Without reinsert, the original note-on would be lost from the EventIdHandler, causing a stuck note. The reinsert logic re-adds it so a future note-off can properly match it.

---

## delayEvent -- Timestamp Manipulation

```cpp
void Message::delayEvent(int samplesToDelay) {
    if (messageHolder == nullptr) {
        reportIllegalCall("delayEvent()", "midi event");
        return;
    }
    messageHolder->addToTimeStamp(samplesToDelay);
}
```

This adds `samplesToDelay` to the event's timestamp. The `HiseEvent::addToTimeStamp()` method:
```cpp
void addToTimeStamp(int delta) noexcept {
    int v = getTimeStamp() + delta;
    v = jmax<int>(0, v);  // Clamp to 0 minimum
    setTimeStamp(v);
}
```

If the resulting timestamp exceeds the current buffer size, the event will be processed in a future buffer. The `HiseEventBuffer::moveEventsBelow()` / `moveEventsAbove()` methods handle splitting events across buffer boundaries.

---

## store() -- Bridging to MessageHolder

```cpp
void Message::store(var messageEventHolder) const {
    if (constMessageHolder == nullptr) {
        reportIllegalCall("store()", "midi event");
        RETURN_VOID_IF_NO_THROW()
    }
    
    ScriptingObjects::ScriptingMessageHolder* holder = 
        dynamic_cast<ScriptingObjects::ScriptingMessageHolder*>(messageEventHolder.getObject());
    
    if (holder != nullptr && constMessageHolder != nullptr) {
        holder->setMessage(*constMessageHolder);
    }
}
```

This copies the current HiseEvent into a `ScriptingMessageHolder` (the C++ class behind the HiseScript `MessageHolder` API class). The `setMessage()` method creates a new `HiseEvent` copy:

```cpp
void setMessage(const HiseEvent &newEvent) { e = HiseEvent(newEvent); }
```

This is a true copy -- the MessageHolder gets its own independent HiseEvent that persists beyond the callback.

---

## sendToMidiOut -- MIDI Output

```cpp
void Message::sendToMidiOut() {
#if USE_BACKEND
    auto mc = getScriptProcessor()->getMainController_();
    auto midiOutputEnabled = dynamic_cast<GlobalSettingManager*>(mc)
        ->getSettingsObject().getSetting(HiseSettings::Project::EnableMidiOut);
    
    if (!midiOutputEnabled) {
        reportScriptError("You need to enable EnableMidiOut in the project settings");
    }
#endif
    
    makeArtificial();  // Must be artificial to send out
    getScriptProcessor()->getMainController_()->sendToMidiOut(*messageHolder);
}
```

Key observations:
1. `sendToMidiOut()` first calls `makeArtificial()` on the event
2. Only artificial events can be sent to MIDI output (asserted in `MainController::sendToMidiOut()`)
3. In backend builds, there's a check that `EnableMidiOut` is enabled in project settings
4. The `MainController::sendToMidiOut()` adds the event to an output buffer under a `SimpleReadWriteLock`

---

## setAllNotesOffCallback -- Realtime Safety Check

```cpp
void Message::setAllNotesOffCallback(var onAllNotesOffCallback) {
#if USE_BACKEND
    if (auto co = dynamic_cast<WeakCallbackHolder::CallableObject*>(onAllNotesOffCallback.getObject())) {
        if (HiseJavascriptEngine::RootObject::RealtimeSafetyInfo::check(co, this, 
            "Message.setAllNotesOffCallback"))
            reportScriptError("Callback is not safe for audio-thread execution");
    }
#endif
    allNotesOffCallback = WeakCallbackHolder(getScriptProcessor(), this, onAllNotesOffCallback, 0);
    allNotesOffCallback.incRefCount();
}
```

In backend builds, the callback is checked for realtime safety before being registered. The callback is invoked synchronously on the audio thread via `callSync()`:

```cpp
void Message::onAllNotesOff() {
    if (allNotesOffCallback)
        allNotesOffCallback.callSync(nullptr, 0, nullptr);
}
```

The callback takes 0 parameters.

---

## setStartOffset -- Sample-Accurate Note Starts

```cpp
void Message::setStartOffset(int newStartOffset) {
    if (constMessageHolder == nullptr) {
        reportIllegalCall("setStartOffset()", "midi event");
        RETURN_VOID_IF_NO_THROW()
    }
    
    if (newStartOffset > UINT16_MAX)
        reportScriptError("Max start offset is 65536 (2^16)");
    
    messageHolder->setStartOffset((uint16)newStartOffset);
}
```

The start offset field is `uint16`, giving a maximum value of 65535 (about 1.36 seconds at 48kHz). This is stored in the HiseEvent alongside the timestamp and tells the sound generator to skip ahead in the sample when the voice starts. It does NOT delay event processing.

---

## Synth.addNoteOn / addNoteOff / addController -- Artificial Event Creation

These methods on the Synth API class create artificial events that feed into the same HiseEvent pipeline.

### Synth.addNoteOn (internalAddNoteOn)

```cpp
int Synth::internalAddNoteOn(int channel, int noteNumber, int velocity, int timeStampSamples, int startOffset) {
    HiseEvent m = HiseEvent(HiseEvent::Type::NoteOn, (uint8)noteNumber, (uint8)velocity, (uint8)channel);
    
    // Timestamp is relative to current event (if in callback) or absolute
    if (auto ce = parentMidiProcessor->getCurrentHiseEvent())
        m.setTimeStamp((int)ce->getTimeStamp() + timeStampSamples);
    else
        m.setTimeStamp(timeStampSamples);
    
    m.setStartOffset((uint16)startOffset);
    m.setArtificial();
    
    // Register with global EventIdHandler (assigns ID)
    parentMidiProcessor->getMainController()->getEventHandler().pushArtificialNoteOn(m);
    
    // Also register with the Message object's local cache
    if (messageObject != nullptr)
        messageObject->pushArtificialNoteOn(m);
    
    // Add to the MIDI buffer for downstream processing
    parentMidiProcessor->addHiseEventToBuffer(m);
    
    return m.getEventId();
}
```

### Synth.addNoteOff

```cpp
void Synth::addNoteOff(int channel, int noteNumber, int timeStampSamples) {
    timeStampSamples = jmax<int>(1, timeStampSamples);  // Minimum timestamp of 1!
    
    HiseEvent m = HiseEvent(HiseEvent::Type::NoteOff, (uint8)noteNumber, 127, (uint8)channel);
    
    // Timestamp relative to current event
    if (auto ce = parentMidiProcessor->getCurrentHiseEvent())
        m.setTimeStamp((int)ce->getTimeStamp() + timeStampSamples);
    else
        m.setTimeStamp(timeStampSamples);
    
    m.setArtificial();
    
    // Get event ID from EventIdHandler (matches to note-on)
    const uint16 eventId = parentMidiProcessor->getMainController()->getEventHandler()
        .getEventIdForNoteOff(m);
    m.setEventId(eventId);
    
    parentMidiProcessor->addHiseEventToBuffer(m);
}
```

Key: `addNoteOff` enforces a minimum timestamp of 1 sample. This ensures the note-off arrives after any note-on at timestamp 0 in the same buffer.

### Synth.addController

```cpp
void Synth::addController(int channel, int number, int value, int timeStampSamples) {
    bool isPitchBend = number == HiseEvent::PitchWheelCCNumber;  // 128
    
    HiseEvent e;
    if (isPitchBend) {
        e = HiseEvent(HiseEvent::Type::PitchBend, 0, 0, (uint8)channel);
        e.setPitchWheelValue(value);
    }
    else if (number == HiseEvent::AfterTouchCCNumber) {  // 129
        e = HiseEvent(HiseEvent::Type::Aftertouch, 0, value, (uint8)channel);
    }
    else {
        e = HiseEvent(HiseEvent::Type::Controller, (uint8)number, (uint8)value, (uint8)channel);
    }
    
    // Timestamp relative to current event
    if (auto ce = parentMidiProcessor->getCurrentHiseEvent())
        e.setTimeStamp((int)ce->getTimeStamp() + timeStampSamples);
    else
        e.setTimeStamp(timeStampSamples);
    
    e.setArtificial();
    parentMidiProcessor->addHiseEventToBuffer(e);
}
```

The same CC number convention applies here: 128 = PitchBend, 129 = Aftertouch. The value range for pitch bend is 0-16383 (14-bit), validated separately.

---

## ScriptingMessageHolder -- The Persistent Counterpart

**Location:** `hi_scripting/scripting/api/ScriptingApiObjects.h` lines 1478-1620

```cpp
class ScriptingObjects::ScriptingMessageHolder : public ConstScriptingObject
```

Unlike `ScriptingApi::Message`, the `ScriptingMessageHolder`:
- Inherits from `ConstScriptingObject` (standard pattern for persistent scripting objects)
- Owns its own `HiseEvent e` member (not a pointer)
- Has no safe-check guards around getter/setter methods (no `ENABLE_SCRIPTING_SAFE_CHECKS`)
- Has additional methods not on Message: `setType()`, `clone()`, `setTimestamp()`, `addToTimestamp()`, `isNoteOn()`, `isNoteOff()`, `isController()`, `dump()`
- Does NOT have: `delayEvent()`, `makeArtificial()`, `makeArtificialOrLocal()`, `isArtificial()`, `sendToMidiOut()`, `setAllNotesOffCallback()`, `store()`

The MessageHolder is created by `Engine.createMessageHolder()` and the `Message.store()` method populates it.

---

## getControllerValue() -- Polymorphic Return

```cpp
var Message::getControllerValue() const {
    if      (constMessageHolder->isController())  return constMessageHolder->getControllerValue();
    else if (constMessageHolder->isAftertouch())  return constMessageHolder->getAfterTouchValue();
    else if (constMessageHolder->isPitchWheel())  return constMessageHolder->getPitchWheelValue();
    else                                          return var::undefined();
}
```

This method returns different value ranges depending on event type:
- Controller CC: 0-127
- Aftertouch: 0-127
- Pitch wheel: 0-16383 (14-bit)

### isMonophonicAfterTouch vs isPolyAftertouch -- Naming Confusion

In the HiseEvent system, both monophonic (channel pressure) and polyphonic aftertouch use `Type::Aftertouch`. The distinction is:
- `isMonophonicAfterTouch()` -> calls `HiseEvent::isChannelPressure()` (which checks `type == Type::Aftertouch`)
- `isPolyAftertouch()` -> also calls `HiseEvent::isAftertouch()` (which checks `type == Type::Aftertouch`)

Both return true for the same event type! The practical distinction must be determined by the note number field -- channel pressure typically has noteNumber == 0 while poly aftertouch has the actual note number. However, from the C++ source, both methods check only the type, not the note number content.

Note: in the MidiMessage-to-HiseEvent conversion, both `isChannelPressure()` and `isAftertouch()` on the incoming MidiMessage map to `Type::Aftertouch`, with the difference that channel pressure copies value from the number byte:
```cpp
if(message.isChannelPressure())
    value = number;
```

---

## Preprocessor Guards

| Guard | Usage |
|-------|-------|
| `ENABLE_SCRIPTING_SAFE_CHECKS` | Most getter/setter methods wrap their null/type checks in this. When disabled, no safety checks occur. |
| `USE_BACKEND` | `sendToMidiOut()` checks EnableMidiOut setting only in backend. `setAllNotesOffCallback()` checks realtime safety only in backend. |
| `FRONTEND_ONLY(x)` | `getEventId()`, `setTransposeAmount()`, `getTransposeAmount()` use this for silent defaults in exported plugins. |
| `HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS` | In `internalAddNoteOn`, optionally subtracts one block from timestamps for backwards compatibility with older patches. |
| `ENABLE_SCRIPTING_BREAKPOINTS` | Breakpoint support in `runScriptCallbacks()`. |

---

## Lifecycle Summary

1. **Script processor created**: `Message` object instantiated in `registerApiClasses()`, registered as global API
2. **Audio buffer arrives**: `EventIdHandler::handleEventIds()` assigns event IDs to all real note-on/note-off pairs
3. **Per-event processing**: For each HiseEvent in the buffer:
   a. `ScopedValueSetter` sets `currentEvent` pointer
   b. `setHiseEvent(m)` points Message's internal pointers at the event
   c. `runScriptCallbacks()` dispatches to onNoteOn/onNoteOff/onController/onTimer
   d. Script reads/modifies the event through Message API
   e. After callback, `ScopedValueSetter` resets `currentEvent`
4. **Event continues downstream**: Modified event (in-place in the buffer) flows to downstream processors
5. **Cleanup**: Message's pointers become dangling after the callback scope -- safe checks prevent post-callback access
