# MessageHolder -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- enrichment prerequisites (MessageHolder has no explicit prerequisite in the table)
- `enrichment/resources/survey/class_survey_data.json` -- MessageHolder entry (createdBy: Engine, seeAlso: Message, UnorderedStack, MidiPlayer)
- `enrichment/base/MessageHolder.json` -- authoritative method list (33 methods)
- `enrichment/resources/explorations/Message.md` -- prerequisite exploration for HiseEvent architecture, event types, EventIdHandler, timestamp bit packing, artificial event system

## Source Files

| File | Role |
|------|------|
| `hi_scripting/scripting/api/ScriptingApiObjects.h` lines 1478-1620 | Class declaration |
| `hi_scripting/scripting/api/ScriptingApiObjects.cpp` lines 5482-5683 | Full implementation |
| `hi_scripting/scripting/api/ScriptingApi.cpp` lines 3490-3493 | Engine.createMessageHolder() factory |
| `hi_scripting/scripting/api/ScriptingApi.cpp` lines 1015-1028 | Message.store() -- populates MessageHolder |
| `hi_scripting/scripting/api/ScriptingApi.cpp` lines 5696-5738 | Synth.addMessageFromHolder() |
| `hi_scripting/scripting/api/ScriptingApiObjects.h` lines 1703-1823 | ScriptUnorderedStack (event stack mode) |
| `hi_scripting/scripting/api/ScriptingApiObjects.cpp` lines 7414-7693 | UnorderedStack event-mode implementation |
| `hi_scripting/scripting/api/ScriptingApiObjects.cpp` lines 6514-6586 | MidiPlayer getEventList/flushMessageList |
| `hi_scripting/scripting/api/ScriptingApiObjects.cpp` lines 622-704 | File.writeMidiFile/loadAsMidiFile (uses MessageHolder arrays) |
| `hi_scripting/scripting/HardcodedScriptProcessor.h` lines 395-474 | ReleaseTriggerScriptProcessor (usage pattern) |

---

## Relationship to Message (ScriptingApi::Message)

MessageHolder and Message are fundamentally different wrappers around the same `HiseEvent` struct. The key architectural differences:

| Aspect | Message | MessageHolder |
|--------|---------|---------------|
| C++ class | `ScriptingApi::Message` | `ScriptingObjects::ScriptingMessageHolder` |
| Base class | `ScriptingObject, ApiClass` | `ConstScriptingObject` |
| HiseEvent storage | `HiseEvent* messageHolder` (pointer to live event) | `HiseEvent e` (owned copy) |
| Lifetime | Transient -- valid only during callback scope | Persistent -- lives as long as the script holds a reference |
| Safety checks | Extensive: null pointer, event type, range validation | None -- no `ENABLE_SCRIPTING_SAFE_CHECKS` guards |
| Write access | Conditional: read-only in some contexts (`constMessageHolder`) | Always writable |
| Namespace | Registered as API class (global `Message` name) | Created as object instances |
| Instance per processor | Singleton (one per script processor) | Multiple instances (create as needed) |

For full HiseEvent struct documentation (memory layout, bit packing, event types, EventIdHandler), see the Message exploration.

---

## Class Declaration

```cpp
class ScriptingMessageHolder : public ConstScriptingObject
{
public:
    ScriptingMessageHolder(ProcessorWithScriptingContent* content);
    ~ScriptingMessageHolder() {};

    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("MessageHolder"); }
    String getDebugName() const override { return "MessageHolder"; };
    String getDebugValue() const override { return dump(); };

    // ... API methods ...

    void setMessage(const HiseEvent &newEvent) { e = HiseEvent(newEvent); }
    HiseEvent getMessageCopy() const { return e; }

private:
    struct Wrapper;
    HiseEvent e;
};
```

**Location:** `hi_scripting/scripting/api/ScriptingApiObjects.h` lines 1478-1620

Key observations:
- The class inherits from `ConstScriptingObject`, the standard base for persistent scripting objects with `addConstant()` and `ADD_API_METHOD_N` support.
- Constructor parameter `(int)HiseEvent::Type::numTypes` = 14, indicating 14 constants will be registered.
- The `HiseEvent e` member is a VALUE, not a pointer. This is the critical distinction from `Message`. The event data is copied into this member and persists independently.
- `setMessage()` creates a copy via the HiseEvent copy constructor: `e = HiseEvent(newEvent)`.
- `getMessageCopy()` returns a full value copy of the internal event, used by external code (Synth.addMessageFromHolder, UnorderedStack, MidiPlayer).
- `getDebugValue()` calls `dump()`, so inspecting the object in the debugger shows the human-readable event dump.

---

## Constructor -- Constants and Method Registration

**Location:** `hi_scripting/scripting/api/ScriptingApiObjects.cpp` lines 5522-5580

### Constants (addConstant calls)

All 14 HiseEvent::Type enum values are exposed:

| Name | Value | HiseEvent::Type |
|------|-------|-----------------|
| `Empty` | 0 | `Empty` |
| `NoteOn` | 1 | `NoteOn` |
| `NoteOff` | 2 | `NoteOff` |
| `Controller` | 3 | `Controller` |
| `PitchBend` | 4 | `PitchBend` |
| `Aftertouch` | 5 | `Aftertouch` |
| `AllNotesOff` | 6 | `AllNotesOff` |
| `SongPosition` | 7 | `SongPosition` |
| `MidiStart` | 8 | `MidiStart` |
| `MidiStop` | 9 | `MidiStop` |
| `VolumeFade` | 10 | `VolumeFade` |
| `PitchFade` | 11 | `PitchFade` |
| `TimerEvent` | 12 | `TimerEvent` |
| `ProgramChange` | 13 | `ProgramChange` |

**Important difference from Message:** Message only exposes a subset of these constants (0-6 plus 10, 11, and two CC constants PITCH_BEND_CC=128, AFTERTOUCH_CC=129). MessageHolder exposes ALL 14 enum values including SongPosition, MidiStart, MidiStop, TimerEvent, and ProgramChange -- but does NOT expose the CC number constants (128/129).

### Typed Method Registrations (ADD_TYPED_API_METHOD_1)

All single-parameter setter methods use typed registration:

| Method | VarTypeChecker | Type |
|--------|---------------|------|
| `setNoteNumber` | `Number` | Number |
| `setVelocity` | `Number` | Number |
| `setControllerNumber` | `Number` | Number |
| `setControllerValue` | `Number` | Number |
| `setChannel` | `Number` | Number |
| `setGain` | `Number` | Number |
| `setType` | `Number` | Number |
| `setTransposeAmount` | `Number` | Number |
| `setFineDetune` | `Number` | Number |
| `setCoarseDetune` | `Number` | Number |
| `setTimestamp` | `Number` | Number |
| `setStartOffset` | `Number` | Number |

### Plain Method Registrations (ADD_API_METHOD_N)

All getter methods and the remaining API:

| Method | Registration |
|--------|-------------|
| `getControllerNumber` | `ADD_API_METHOD_0` |
| `getControllerValue` | `ADD_API_METHOD_0` |
| `getNoteNumber` | `ADD_API_METHOD_0` |
| `getVelocity` | `ADD_API_METHOD_0` |
| `ignoreEvent` | `ADD_API_METHOD_1` |
| `getEventId` | `ADD_API_METHOD_0` |
| `getChannel` | `ADD_API_METHOD_0` |
| `getGain` | `ADD_API_METHOD_0` |
| `isMonophonicAfterTouch` | `ADD_API_METHOD_0` |
| `getMonophonicAftertouchPressure` | `ADD_API_METHOD_0` |
| `setMonophonicAfterTouchPressure` | `ADD_API_METHOD_1` |
| `isPolyAftertouch` | `ADD_API_METHOD_0` |
| `getPolyAfterTouchNoteNumber` | `ADD_API_METHOD_0` |
| `getPolyAfterTouchPressureValue` | `ADD_API_METHOD_0` |
| `setPolyAfterTouchNoteNumberAndPressureValue` | `ADD_API_METHOD_2` |
| `getTransposeAmount` | `ADD_API_METHOD_0` |
| `getCoarseDetune` | `ADD_API_METHOD_0` |
| `getFineDetune` | `ADD_API_METHOD_0` |
| `getTimestamp` | `ADD_API_METHOD_0` |
| `isNoteOn` | `ADD_API_METHOD_0` |
| `isNoteOff` | `ADD_API_METHOD_0` |
| `isController` | `ADD_API_METHOD_0` |
| `clone` | `ADD_API_METHOD_0` |
| `dump` | `ADD_API_METHOD_0` |

### Wrapper Struct

All use plain `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` macros. No typed wrappers in the wrapper struct.

---

## Factory / obtainedVia

### Engine.createMessageHolder()

```cpp
ScriptingObjects::ScriptingMessageHolder* ScriptingApi::Engine::createMessageHolder()
{
    return new ScriptingObjects::ScriptingMessageHolder(getScriptProcessor());
}
```

Simple factory -- creates a new instance with a default-constructed HiseEvent (type Empty, all fields zero).

### Message.store() -- Populating from live callback

```cpp
void Message::store(var messageEventHolder) const {
    ScriptingObjects::ScriptingMessageHolder* holder = 
        dynamic_cast<ScriptingObjects::ScriptingMessageHolder*>(messageEventHolder.getObject());
    
    if (holder != nullptr && constMessageHolder != nullptr) {
        holder->setMessage(*constMessageHolder);
    }
}
```

This copies the current live HiseEvent (from the callback) into the MessageHolder's owned `e` member. The MessageHolder then persists this data independently of the callback lifetime.

### Other creation points

MessageHolder objects are also created internally by:
- `MidiPlayer.getEventList()` / `getEventListFromSequence()` -- creates an array of MessageHolder objects from MIDI sequence data
- `File.loadAsMidiFile()` -- creates MessageHolder objects from MIDI file data
- `UnorderedStack.copyTo()` -- when copying event stack to array, creates new MessageHolder per event
- `UnorderedStack.removeIfEqual()` -- writes matched event data back into the holder passed as argument
- `clone()` method -- creates a new MessageHolder with copied event data

---

## Methods Unique to MessageHolder (not on Message)

### setType(int type)

```cpp
void ScriptingObjects::ScriptingMessageHolder::setType(int type)
{
    if(isPositiveAndBelow(type, (int)HiseEvent::Type::numTypes))
        e.setType((HiseEvent::Type)type);
    else
        reportScriptError("Unknown Type: " + String(type));
}
```

Allows changing the event type after creation. The type is validated against the full range (0 to numTypes-1 = 13). This is how you construct arbitrary event types from scratch -- create a MessageHolder, then call `setType()` with the appropriate constant.

### setTimestamp(int timestampSamples)

```cpp
void ScriptingObjects::ScriptingMessageHolder::setTimestamp(int timestampSamples) {
    e.setTimeStamp(timestampSamples);
}
```

Directly sets the sample timestamp. The `HiseEvent::setTimeStamp()` preserves the upper 2 flag bits (artificial and ignored) while writing the lower bits. Message has `delayEvent()` instead, which adds a delta to the existing timestamp.

### addToTimestamp(int deltaSamples)

```cpp
void ScriptingObjects::ScriptingMessageHolder::addToTimestamp(int deltaSamples) {
    e.addToTimeStamp((int16)deltaSamples);
}
```

Adds a delta to the current timestamp. Note the `(int16)` cast -- this limits the delta range to -32768..32767 samples. The underlying `HiseEvent::addToTimeStamp()` clamps the result to >= 0.

### isNoteOn() / isNoteOff() / isController()

```cpp
bool ScriptingObjects::ScriptingMessageHolder::isNoteOn() const { return e.isNoteOn(); }
bool ScriptingObjects::ScriptingMessageHolder::isNoteOff() const { return e.isNoteOff(); }
bool ScriptingObjects::ScriptingMessageHolder::isController() const { 
    return e.isController() || e.isPitchWheel() || e.isAftertouch(); 
}
```

Type-checking methods. Note that `isController()` returns true for CC messages, pitch wheel, AND aftertouch -- matching the HISE convention that all three are "controller-like" events (same as how onController callback receives all three).

### clone()

```cpp
juce::var ScriptingObjects::ScriptingMessageHolder::clone()
{
    auto no = new ScriptingMessageHolder(getScriptProcessor());
    no->setMessage(e);
    return var(no);
}
```

Creates a new independent MessageHolder with a full copy of the HiseEvent. Returns the new holder as a var (reference-counted object).

### dump()

```cpp
String ScriptingObjects::ScriptingMessageHolder::dump() const
{
    String x;
    x << "Type: " << e.getTypeAsString() << ", ";
    x << "Channel: " << String(e.getChannel()) << ", ";

    if (e.isPitchWheel())
    {
        x << "Value: " << String(e.getPitchWheelValue()) << ", ";
    }
    else
    {
        x << "Number: " << String(e.getNoteNumber()) << ", ";
        x << "Value: " << String(e.getVelocity()) << ", ";
        x << "EventId: " << String(e.getEventId()) << ", ";
    }

    x << "Timestamp: " << String(e.getTimeStamp()) << ", ";
    return x;
}
```

Returns a human-readable string. Note: for pitch wheel events, it shows the 14-bit pitch wheel value instead of note number/velocity/event ID. This is also what shows in the debugger view (`getDebugValue()` calls `dump()`).

---

## Methods Shared with Message -- Implementation Differences

MessageHolder implementations are direct pass-throughs to HiseEvent with no safety checks:

### No Callback Scope Validation

Unlike Message, MessageHolder never checks `constMessageHolder == nullptr` or validates event types. Every method directly accesses `e`:

```cpp
int ScriptingObjects::ScriptingMessageHolder::getNoteNumber() const { return (int)e.getNoteNumber(); }
int ScriptingObjects::ScriptingMessageHolder::getVelocity() const { return e.getVelocity(); }
int ScriptingObjects::ScriptingMessageHolder::getChannel() const { return (int)e.getChannel(); }
```

This means you can call `getNoteNumber()` on a Controller event without error -- it will return whatever value is in the `number` byte field, which for a CC event would be the controller number.

### getControllerNumber() -- No Type Guard

```cpp
var ScriptingObjects::ScriptingMessageHolder::getControllerNumber() const 
{ 
    return (int)e.getControllerNumber();
}
```

Unlike Message's version (which checks event type and may return undefined), MessageHolder's version always returns the value from `HiseEvent::getControllerNumber()`. That HiseEvent method returns virtual CC numbers: 128 for PitchBend, 129 for Aftertouch, otherwise the actual CC number.

### getControllerValue() -- Simplified

```cpp
var ScriptingObjects::ScriptingMessageHolder::getControllerValue() const 
{ 
    if (e.isPitchWheel())
        return e.getPitchWheelValue();
    else
        return (int)e.getControllerValue(); 
}
```

The pitch wheel check is preserved (returns 14-bit value for pitch wheel), but there's no type guard or undefined return for non-controller events.

### setControllerNumber() -- Type Coercion

```cpp
void ScriptingObjects::ScriptingMessageHolder::setControllerNumber(int newControllerNumber) 
{ 
    if (newControllerNumber == HiseEvent::AfterTouchCCNumber)        // 129
        e.setType(HiseEvent::Type::Aftertouch);
    else if (newControllerNumber == HiseEvent::PitchWheelCCNumber)   // 128
        e.setType(HiseEvent::Type::PitchBend);
    else
        e.setControllerNumber(newControllerNumber);
}
```

Setting controller number to 128 or 129 actually changes the event TYPE to PitchBend or Aftertouch respectively. This enables creating these event types through the controller number API.

### setControllerValue() -- Pitch Wheel Awareness

```cpp
void ScriptingObjects::ScriptingMessageHolder::setControllerValue(int newControllerValue) 
{ 
    if (e.isPitchWheel())
        e.setPitchWheelValue(newControllerValue);
    else
        e.setControllerValue(newControllerValue); 
}
```

If the event is currently a pitch wheel, the value is stored as a 14-bit pitch wheel value (split across number and value bytes). Otherwise stored as a standard CC value.

---

## Methods Message Has But MessageHolder Does NOT

These methods are absent from MessageHolder because they relate to the live callback context:

| Missing Method | Why |
|---------------|-----|
| `delayEvent()` | Delays event in the processing buffer -- MessageHolder is not in a buffer |
| `makeArtificial()` | Converts live event to artificial, updates EventIdHandler -- MessageHolder has no live event context |
| `makeArtificialOrLocal()` | Same as above |
| `isArtificial()` | Checks artificial flag bit -- MessageHolder could theoretically expose this but doesn't |
| `sendToMidiOut()` | Sends to MIDI output -- requires live processing context |
| `store()` | Copies live event into a MessageHolder -- MessageHolder IS the storage target |
| `setAllNotesOffCallback()` | Registers callback for AllNotesOff -- MessageHolder has no callback system |
| `isProgramChange()` | Type check -- MessageHolder has `isNoteOn`/`isNoteOff`/`isController` but not isProgramChange |
| `getProgramChangeNumber()` | Not exposed on MessageHolder |

---

## Synth.addMessageFromHolder() -- Re-injecting Events

**Location:** `hi_scripting/scripting/api/ScriptingApi.cpp` lines 5696-5738

```cpp
int ScriptingApi::Synth::addMessageFromHolder(var messageHolder)
{
    ScriptingObjects::ScriptingMessageHolder* m = 
        dynamic_cast<ScriptingObjects::ScriptingMessageHolder*>(messageHolder.getObject());

    if (m != nullptr)
    {
        HiseEvent e = m->getMessageCopy();

        if (e.getType() != HiseEvent::Type::Empty)
        {
            e.setArtificial();

            if (e.isNoteOn())
            {
                parentMidiProcessor->getMainController()->getEventHandler().pushArtificialNoteOn(e);
                if (messageObject != nullptr)
                    messageObject->pushArtificialNoteOn(e);
                parentMidiProcessor->addHiseEventToBuffer(e);
                return e.getEventId();
            }
            else if (e.isNoteOff())
            {
                e.setEventId(parentMidiProcessor->getMainController()->getEventHandler()
                    .getEventIdForNoteOff(e));
                parentMidiProcessor->addHiseEventToBuffer(e);
                return e.getTimeStamp();
            }
            else
            {
                parentMidiProcessor->addHiseEventToBuffer(e);
                return 0;
            }
        }
        else reportScriptError("Event is empty");
    }
    else reportScriptError("Not a message holder");
}
```

### Key behaviors:

1. **Makes a copy** -- `getMessageCopy()` returns a value copy of the HiseEvent. The original MessageHolder is not modified.
2. **Always sets artificial** -- Every event injected through this path becomes artificial (bit 31 of timestamp set).
3. **NoteOn path**: Registers with `EventIdHandler.pushArtificialNoteOn()` (gets new event ID), registers with `Message.pushArtificialNoteOn()` (local cache), adds to buffer. Returns the **event ID**.
4. **NoteOff path**: Gets matching event ID via `getEventIdForNoteOff()`. Adds to buffer. Returns the **timestamp** (not event ID).
5. **Other events** (CC, pitch bend, etc.): Adds to buffer. Returns 0.
6. **Error if empty**: Reports error if the MessageHolder contains a default-constructed (Empty type) event.

### Return value semantics

| Event Type | Return Value |
|-----------|-------------|
| NoteOn | Event ID (for later note-off matching) |
| NoteOff | Timestamp |
| Other | 0 |

---

## UnorderedStack Integration (Event Stack Mode)

The `UnorderedStack` can operate in "event stack" mode, where it stores `HiseEvent` objects internally and uses `MessageHolder` as the scripting-side interface.

### Setup

```cpp
void ScriptUnorderedStack::setIsEventStack(bool shouldBeEventStack, var eventCompareFunction)
```

The compare function can be:
- A built-in constant (BitwiseEqual=0, EventId=1, NoteNumberAndVelocity=2, NoteNumberAndChannel=3, EqualData=4)
- A custom JavaScript function taking two MessageHolder arguments

### insert(value) -- Push event into stack

When `isEventStack` is true:
```cpp
if (auto m = dynamic_cast<ScriptingMessageHolder*>(value.getObject()))
    return eventData.insert(m->getMessageCopy());
```
Extracts the HiseEvent from the MessageHolder and stores a copy in the internal `hise::UnorderedStack<HiseEvent, 128>`.

### storeEvent(index, holder) -- Read event from stack

```cpp
if (auto m = dynamic_cast<ScriptingMessageHolder*>(holder.getObject()))
{
    if (isPositiveAndBelow(index, size()))
    {
        m->setMessage(eventData[index]);
        return true;
    }
    return false;
}
```
Copies the HiseEvent at the given index into the provided MessageHolder.

### removeIfEqual(holder) -- Remove matching event

```cpp
auto idx = getIndexForEvent(holder);
if (idx != -1)
{
    auto eventFromStack = eventData[idx];
    eventData.removeElement(idx);
    dynamic_cast<ScriptingMessageHolder*>(holder.getObject())->setMessage(eventFromStack);
    return true;
}
```
Finds the matching event using the configured compare function, removes it from the stack, and **writes the removed event back into the holder**. This is a destructive read -- the event is both removed and returned.

### copyTo(target) -- Export stack to array

When target is an Array and the stack is in event mode:
```cpp
for (const auto& e : eventData)
{
    auto m = new ScriptingMessageHolder(getScriptProcessor());
    m->setMessage(e);
    target.append(var(m));
}
```
Creates new MessageHolder objects for each event in the stack.

### Compare Functions

The compare function determines how `remove()`, `removeIfEqual()`, and `contains()` find matching events:

| Constant | Algorithm |
|----------|-----------|
| `BitwiseEqual` (0) | `e1 == e2` -- all 16 bytes must match |
| `EventId` (1) | `e1.getEventId() == e2.getEventId()` |
| `NoteNumberAndVelocity` (2) | Both must be NoteOn, same note number and velocity |
| `NoteNumberAndChannel` (3) | Same note number AND same channel |
| `EqualData` (4) | Defined as enum but falls through to `default: jassertfalse` in the MCF template -- effectively unimplemented |
| Custom function | Calls a JavaScript function with two MessageHolder arguments, expects boolean return |

For custom compare functions, the stack maintains an internal `compareHolder` (a `ScriptingMessageHolder*`) that it populates with each stack element for comparison. The function receives `[compareHolder, searchHolder]` as arguments.

---

## MidiPlayer Integration

### getEventList() / getEventListFromSequence()

```cpp
var ScriptingObjects::ScriptedMidiPlayer::getEventListFromSequence(int sequenceIndexOneBased)
{
    Array<var> eventHolders;

    if(auto seq = getPlayer()->getSequenceWithIndex(sequenceIndexOneBased))
    {
        auto sr = getPlayer()->getSampleRate();
        auto bpm = getPlayer()->getMainController()->getBpm();

        seq->setTimeStampEditFormat(useTicks ? 
            HiseMidiSequence::TimestampEditFormat::Ticks : 
            HiseMidiSequence::TimestampEditFormat::Samples);

        auto list = seq->getEventList(sr, bpm);

        for (const auto& e : list)
        {
            ScopedPointer<ScriptingMessageHolder> holder = new ScriptingMessageHolder(getScriptProcessor());
            holder->setMessage(e);
            eventHolders.add(holder.release());
        }
    }

    return var(eventHolders);
}
```

Returns an Array of MessageHolder objects, one per event in the MIDI sequence. Timestamp format depends on `setUseTimestampInTicks()` -- either sample-based or tick-based (PPQN).

### flushMessageList() / flushMessageListToSequence()

```cpp
void ScriptingObjects::ScriptedMidiPlayer::flushMessageListToSequence(var messageList, int sequenceIndex)
{
    Array<HiseEvent> events;

    if (auto ar = messageList.getArray())
    {
        events.ensureStorageAllocated(messageList.size());

        for (auto e : *ar)
        {
            if (auto holder = dynamic_cast<ScriptingMessageHolder*>(e.getObject()))
                events.add(holder->getMessageCopy());
            else
                reportScriptError("Illegal item in message list: " + e.toString());
        }
    }

    // ... writes events back to sequence
}
```

Takes an Array of MessageHolder objects and writes them back into the MIDI sequence. This is the round-trip: `getEventList()` returns MessageHolder array -> script manipulates events -> `flushMessageList()` writes them back.

---

## File.writeMidiFile / File.loadAsMidiFile

### File.loadAsMidiFile(trackIndex)

```cpp
auto list = newSequence->getEventList(44100.0, 120.0);

Array<var> eventList;
for (auto e : list)
{
    auto eh = new ScriptingMessageHolder(getScriptProcessor());
    eh->setMessage(e);
    eventList.add(var(eh));
}

auto returnObj = new DynamicObject();
returnObj->setProperty("TimeSignature", obj);
returnObj->setProperty("Events", var(eventList));
return var(returnObj);
```

Returns a JSON object with `TimeSignature` and `Events` properties. `Events` is an Array of MessageHolder objects.

### File.writeMidiFile(eventList, metadata)

```cpp
for (auto e : *eventList.getArray())
{
    if (auto eh = dynamic_cast<ScriptingMessageHolder*>(e.getObject()))
        events.add(eh->getMessageCopy());
}
```

Converts MessageHolder array back to HiseEvent array for MIDI file writing.

---

## HardcodedScriptProcessor Usage Pattern (ReleaseTriggerScriptProcessor)

This C++ hardcoded script processor demonstrates the canonical MessageHolder usage pattern:

```cpp
// Initialization: create 128 MessageHolders (one per note) plus a reusable "current" holder
ReferenceCountedArray<ScriptingObjects::ScriptingMessageHolder> messageHolders;
ReferenceCountedObjectPtr<ScriptingObjects::ScriptingMessageHolder> currentMessageHolder;
var currentMessageVar;

// In constructor:
currentMessageHolder = Engine.createMessageHolder();
currentMessageVar = var(currentMessageHolder.get());
for (int i = 0; i < 128; i++)
    messageHolders.add(Engine.createMessageHolder());

// onNoteOn: store the live event for later retrieval
Message.ignoreEvent(true);
messageHolders[number]->setMessage(*getCurrentHiseEvent());

// onNoteOff: retrieve stored event, modify it, re-inject
auto onEvent = messageHolders[noteNumber]->getMessageCopy();
onEvent.setVelocity((uint8)v);
onEvent.ignoreEvent(false);
onEvent.setTimeStamp((int)Message.getTimestamp());
currentMessageHolder->setMessage(onEvent);
Synth.addMessageFromHolder(currentMessageVar);
```

This shows the complete lifecycle:
1. Create MessageHolder objects during init
2. Store live events from callbacks using `setMessage()`
3. Retrieve stored events using `getMessageCopy()`
4. Modify the HiseEvent directly (C++ level)
5. Put modified event into a holder
6. Re-inject via `Synth.addMessageFromHolder()`

Note: In HiseScript, step 3-5 would be done through the MessageHolder API methods rather than direct HiseEvent manipulation.

---

## Threading Model

MessageHolder has **no thread safety mechanisms**:

- No locks, no atomic operations
- No `SimpleReadWriteLock` or `SpinLock`
- The `HiseEvent e` member is a plain 16-byte value with no synchronization
- Reference counting (from ConstScriptingObject -> ReferenceCountedObject) is thread-safe for the pointer, but not for the event data

**Practical implications:**
- MessageHolder is safe to create on any thread
- Reading/writing the same MessageHolder from multiple threads concurrently is undefined behavior
- The typical pattern (create on init thread, use on audio thread via `Synth.addMessageFromHolder`) is safe because the audio thread gets a COPY via `getMessageCopy()`
- When used with UnorderedStack, the stack's internal `hise::UnorderedStack<HiseEvent, 128>` stores copies, not references to the MessageHolder

---

## No Safety Checks

Unlike Message, MessageHolder has zero runtime safety validation:

- No null pointer checks (the event is always valid since it's an owned value)
- No event type checks (calling `getNoteNumber()` on a CC event returns whatever is in the number byte)
- No range validation on setters (except `setType()` which validates against numTypes)
- No `ENABLE_SCRIPTING_SAFE_CHECKS` guards
- No `FRONTEND_ONLY` silent returns

This is by design -- MessageHolder is a dumb data container. The user is responsible for ensuring they're reading/writing appropriate fields for the event type.

---

## Preprocessor Guards

None. MessageHolder has no preprocessor-conditional code. All methods are unconditionally compiled in both backend and frontend builds.

---

## Default State

A newly created MessageHolder (from `Engine.createMessageHolder()`) contains a default-constructed `HiseEvent`:
- Type: `Empty` (0)
- Channel: 0
- Number: 0
- Value: 0
- Event ID: 0
- Timestamp: 0
- All other fields: 0

Attempting to use `Synth.addMessageFromHolder()` with a default (Empty) MessageHolder will trigger the error: "Event is empty". You must set at least the type before injecting.
