# HiseEvent System Reference

Distilled from C++ source for the scriptnode node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_tools/hi_tools/HiseEventBuffer.h`
- `hi_dsp_library/snex_basics/snex_Types.h` (VoiceDataStack)
- `hi_dsp_library/node_api/nodes/processors.h` (wrap::event, static_functions::event)
- `hi_dsp_library/node_api/nodes/OpaqueNode.h` (shouldProcessHiseEvent wiring)
- `hi_dsp_library/node_api/nodes/prototypes.h` (function pointer typedefs)
- `hi_dsp_library/node_api/helpers/node_macros.h` (SN_EMPTY_HANDLE_EVENT, SN_DEFAULT_HANDLE_EVENT)
- `hi_dsp_library/dsp_nodes/EventNodes.h` (control::midi node)
- `hi_dsp_library/dsp_basics/logic_classes.h` (midi_logic namespace)
- `hi_dsp_library/node_api/nodes/Base.h` (polyphonic_base)

Related infrastructure: `core.md` (ProcessData, PrepareSpecs, PolyHandler, VoiceDataStack)

---

## 1. HiseEvent Class

HiseEvent is a fixed-size (128-bit / 16 bytes) event type that replaces JUCE's
MidiMessage for all internal HISE audio-path processing. It is defined in
`hi_tools/hi_tools/HiseEventBuffer.h`.

### Memory Layout

```
DWord 1 (bytes 0-3):   type (uint8), channel (uint8), number (uint8), value (uint8)
DWord 2 (bytes 4-7):   transposeValue (int8), gain (int8), semitones (int8), cents (int8)
DWord 3 (bytes 8-11):  eventId (uint16), startOffset (uint16)
DWord 4 (bytes 12-15): timestamp (uint32)
```

Total: 16 bytes, aligned to 16 bytes on non-Windows platforms.
The fixed size enables trivial memset/memcpy for clearing and copying buffers.

### Key Advantages Over MidiMessage

- Fixed 128-bit size (no heap allocation, no SysEx support)
- Unique EventID for note-on/note-off association
- 128 channels instead of MIDI's 16
- Built-in transpose amount (avoids stuck notes from note-number changes)
- Extra fields: gain (dB), coarse detune (semitones), fine detune (cents)
- Additional event types: VolumeFade, PitchFade, TimerEvent
- Baked-in timestamp (sample offset within buffer)
- Artificial flag to distinguish script-generated from real MIDI events

---

## 2. Event Types

```cpp
enum class Type : uint8
{
    Empty = 0,       // Default-constructed, isEmpty() returns true
    NoteOn,          // Gets unique EventID on creation
    NoteOff,         // Shares EventID with matching NoteOn
    Controller,      // MIDI CC message
    PitchBend,       // 14-bit pitch bend
    Aftertouch,      // Channel pressure and polyphonic aftertouch
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

### Type Categories

**Standard MIDI types** (come from external MIDI input):
NoteOn, NoteOff, Controller, PitchBend, Aftertouch, AllNotesOff,
SongPosition, MidiStart, MidiStop, ProgramChange

**Internal HISE types** (created programmatically):
VolumeFade, PitchFade, TimerEvent

**Type-check methods:**
- `isNoteOn(returnTrueForVelocity0)` -- note: velocity-0 notes are NoteOff by default
- `isNoteOff()`
- `isNoteOnOrOff()`
- `isController()` / `isControllerOfType(ccNumber)`
- `isPitchWheel()`
- `isAftertouch()` / `isChannelPressure()` (same check: type == Aftertouch)
- `isAllNotesOff()`
- `isMidiStart()` / `isMidiStop()` / `isSongPositionPointer()`
- `isProgramChange()`
- `isVolumeFade()` / `isPitchFade()` / `isTimerEvent()`
- `isEmpty()` -- type == Empty

### Virtual CC Numbers

For APIs that map event types to CC numbers:
- `PitchWheelCCNumber = 128`
- `AfterTouchCCNumber = 129`

---

## 3. Core Event Properties

### Note Data (NoteOn/NoteOff)

| Method | Returns | Notes |
|--------|---------|-------|
| `getNoteNumber()` | int (0-127) | Raw note number |
| `getTransposeAmount()` | int | Transpose offset (int8) |
| `getNoteNumberIncludingTransposeAmount()` | int | number + transposeValue |
| `getVelocity()` | uint8 (0-127) | Raw velocity |
| `getFloatVelocity()` | float (0.0-1.0) | velocity / 127.0f |
| `getGain()` | int | Per-note gain in dB (int8) |
| `getGainFactor()` | float | Decibels::decibelsToGain(gain) |
| `getCoarseDetune()` | int | Semitone detune (int8) |
| `getFineDetune()` | int | Cent detune (int8) |
| `getFrequency()` | double | Full frequency in Hz (uses all properties) |
| `getPitchFactorForEvent()` | double | Pitch ratio 0.5..2.0 |

### Transpose System

The transpose amount is stored separately from the note number. When transposing:
- Set `setTransposeAmount(delta)` on the note-on
- The note-off automatically matches because it shares the EventID
- No need to manually adjust note-off numbers to prevent stuck notes
- Use `getNoteNumberIncludingTransposeAmount()` for the effective pitch

### Channel

- `getChannel()` / `setChannel(n)` -- HISE supports 256 channels (uint8), not just 16

### Timestamp

- `getTimeStamp()` -- sample offset from current buffer start
- `setTimeStamp(n)` / `addToTimeStamp(delta)`
- If timestamp > buffer size, the event is delayed to a future buffer
- Events in a buffer should be sorted by timestamp (ascending)

### Start Offset

- `getStartOffset()` / `setStartOffset(n)` -- tells the voice to skip N samples
  at voice start (e.g., skip attack phase of a sample). This is different from
  timestamp: timestamp delays the event, startOffset skips into the sound.

---

## 4. Event ID System

Every NoteOn event receives a unique EventID (uint16) assigned by the
EventIdHandler. The matching NoteOff receives the same EventID.

### Properties

- Stored as uint16, wraps around at 65536
- Auto-assigned by `EventIdHandler::handleEventIds()` during MIDI input processing
- Used by VolumeFade/PitchFade to target specific voices
- Used by polyphonic nodes to route events to the correct voice

### Key Consideration

Because EventID is uint16 and wraps, you cannot assume older notes have lower
IDs. The ID is only meaningful for matching note-on/note-off pairs and targeting
active voices.

### Artificial Events

- `setArtificial()` -- marks an event as script-generated
- `isArtificial()` -- returns true if created within HISE (not from MIDI input)
- Events from external MIDI are always non-artificial, even if a previous MIDI
  processor modified them
- The `HiseEventBuffer::Iterator` can skip artificial events to prevent
  recursive processing loops

### Ignored Events

- `ignoreEvent(true)` -- marks event to be skipped during processing
- `isIgnored()` -- check the ignored flag
- Ignored events remain in the buffer (not cleared), just skipped by iterators

---

## 5. HiseEventBuffer

A fixed-size buffer holding up to 256 HiseEvents (`HISE_EVENT_BUFFER_SIZE`).

### Storage

```cpp
HiseEvent buffer[256];   // fixed array, 16 bytes each = 4KB total
int numUsed = 0;         // number of active events
```

### Key Methods

| Method | Description |
|--------|-------------|
| `clear()` | Reset buffer (memset) |
| `isEmpty()` / `getNumUsed()` | Query state |
| `addEvent(HiseEvent)` | Append event |
| `addEvent(MidiMessage, sampleNumber)` | Convert and append |
| `addEvents(MidiBuffer)` | Batch convert from JUCE |
| `addEvents(HiseEventBuffer)` | Merge buffers |
| `getEvent(index)` | Get copy by index |
| `sortTimestamps()` | Sort events by timestamp |
| `subtractFromTimeStamps(delta)` | Adjust all timestamps |
| `moveEventsBelow(target, threshold)` | Move events with timestamp < threshold |
| `moveEventsAbove(target, threshold)` | Move events with timestamp >= threshold |

### Iteration

**Range-based for loop** (via begin()/end()):
```cpp
for (auto& e : eventBuffer)
{
    if (e.isNoteOn())
        // ...
}
```

**Iterator class** (with filtering):
```cpp
HiseEventBuffer::Iterator iter(buffer);
HiseEvent e;
int pos;
while (iter.getNextEvent(e, pos, skipIgnored, skipArtificial))
{
    // e is a copy; pos is the timestamp
}
```

The Iterator also provides pointer-based access:
- `getNextConstEventPointer(skipIgnored, skipArtificial)` -- read-only
- `getNextEventPointer(skipIgnored, skipArtificial)` -- read-write

### EventStack

A small stack (16 slots) used internally for tracking active note-on events:
- `push(event)` / `pop()` -- LIFO access
- `peekNoteOnForEventId(id, event)` -- find without removing
- `popNoteOnForEventId(id, event)` -- find and remove

---

## 6. EventIdHandler

Manages EventID assignment and note-on/note-off matching. One instance per
MainController, operating on the master event buffer.

### Core Operations

- `handleEventIds()` -- scans the master buffer, assigns sequential EventIDs to
  note-on events, matches note-off events to their note-on by note number and channel
- `getEventIdForNoteOff(noteOff)` -- finds the matching note-on's EventID
- `pushArtificialNoteOn(noteOn)` -- registers a script-created note-on for matching
- `popNoteOnFromEventId(id)` -- retrieves and removes the note-on for an EventID
- `isArtificialEventId(id)` -- checks if an EventID belongs to an artificial event

### Internal Storage

```cpp
uint16 currentEventId;                    // monotonically increasing counter
HiseEvent realNoteOnEvents[16][128];      // real note-ons indexed by [channel][note]
HeapBlock<HiseEvent> artificialEvents;    // heap storage for artificial note-ons
uint16 lastArtificialEventIds[16][128];   // last artificial ID per [channel][note]
UnorderedStack<HiseEvent, 256> overlappingNoteOns;  // for overlapping same-note events
```

### Choke Groups

The EventIdHandler also manages choke group notifications via `ChokeListener`:
- `sendChokeMessage(source, event)` -- notifies all listeners in the same choke group
- Listeners implement `chokeMessageSent()` to respond (e.g., kill voices)

---

## 7. How Scriptnode Nodes Receive Events

### The handleHiseEvent Callback

Nodes that process MIDI events implement:
```cpp
void handleHiseEvent(HiseEvent& e)
```

The event is passed by non-const reference -- nodes can modify it (e.g., ignore
it, change velocity). The modification propagates to subsequent nodes in the chain.

### IsProcessingHiseEvent Property

A node signals that it needs MIDI events via the `IsProcessingHiseEvent`
CustomNodeProperty. This is set in three ways:

1. **polyphonic_base constructor** -- automatically sets `IsProcessingHiseEvent`
   (default `addProcessEventFlag=true`):
   ```cpp
   polyphonic_base(id, true);  // sets IsPolyphonic AND IsProcessingHiseEvent
   ```

2. **Explicit registration** in constructor:
   ```cpp
   cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::IsProcessingHiseEvent);
   ```

3. **Compile-time trait** via `isProcessingHiseEvent()` method:
   ```cpp
   constexpr bool isProcessingHiseEvent() const { return true; }
   // or:
   static constexpr bool IsProcessingHiseEvent() { return true; }
   ```

During `OpaqueNode::create<T>()`, this trait is queried:
```cpp
if constexpr (prototypes::check::isProcessingHiseEvent<T>::value)
    shouldProcessHiseEvent = t->isProcessingHiseEvent();
```

The `shouldProcessHiseEvent` flag on OpaqueNode controls whether the runtime
forwards events to this node.

### Empty vs. Active Event Handling

- `SN_EMPTY_HANDLE_EVENT` -- defines `void handleHiseEvent(HiseEvent&) {}` (no-op)
- `SN_DEFAULT_HANDLE_EVENT(T)` -- forwards to `obj.handleHiseEvent(e)`

Nodes with SN_EMPTY_HANDLE_EVENT still receive the call if their wrapper sets
IsProcessingHiseEvent; they just do nothing with it.

---

## 8. Event Processing in the Audio Path

### wrap::event -- Sample-Accurate Event Splitting

The `wrap::event<T>` wrapper enables sample-accurate MIDI processing. It splits
the audio buffer at each event's timestamp, interleaving audio processing with
event handling.

```
wrap::event<T>
  - isProcessingHiseEvent() always returns true
  - process() uses static_functions::event::process()
  - handleHiseEvent() forwards to inner obj
```

The `static_functions::event::process()` algorithm:

1. Extract events from ProcessData via `toEventData()`
2. If no events: call process() on the full buffer directly
3. If events exist: use `ChunkableProcessData` to split the buffer:
   a. For each non-ignored event (in timestamp order):
      - Process audio samples from last position to event timestamp
      - Call `handleHiseEvent()` for the event
      - Advance position
   b. Process remaining samples after last event
4. Child nodes do NOT receive events in their ProcessData -- events are
   stripped out (`IncludeHiseEvents = false` in the ChunkableProcessData)

This ensures that parameter changes triggered by MIDI events take effect at
the exact sample position of the event, not at buffer boundaries.

### Container Event Propagation

Different container types propagate events differently:

- **chain** (`Container_Chain.h`): Calls `handleHiseEvent(e)` on all children
  sequentially. Events pass through in order -- modifications by one node
  affect subsequent nodes (same reference).

- **split** (`Container_Split.h`): Makes a copy of each event before forwarding
  to children. Each branch gets an independent copy.

- **multi** (`Container_Multi.h`): Same as split -- copies events before
  forwarding to each child.

### Polyphonic Event Routing (VoiceDataStack)

For polyphonic processing, the `VoiceDataStack` (in snex_Types.h) routes events
to the correct voice(s):

**Note-off:** Matched by EventID to the voice that received the corresponding
note-on. Sets `ScopedVoiceSetter` so the node processes the correct voice state.

**AllNotesOff:** Converted to individual note-off events for each active voice,
dispatched with the correct voice setter.

**CC / PitchBend / Aftertouch:** If voices are active, forwarded to all voices
matching the event's channel. If no voices are active, forwarded once without
voice scoping.

**Other events (non-note-on):** Forwarded to all active voices with voice scoping.

**Note-on:** NOT forwarded through handleHiseEvent routing. Instead handled via
`startVoice()`, which:
1. Stores the voice-to-event mapping
2. Calls `reset()` on the node (with ScopedNoReset to protect envelopes)
3. Calls `handleHiseEvent()` with the note-on event

---

## 9. Static Factory Methods

HiseEvent provides static factory methods for internal event types:

```cpp
// Volume fade: ramps voice gain to targetValue (dB) over fadeTimeMilliseconds
static HiseEvent createVolumeFade(uint16 eventId, int fadeTimeMs, int8 targetValue);

// Pitch fade: ramps voice pitch over fadeTimeMilliseconds
static HiseEvent createPitchFade(uint16 eventId, int fadeTimeMs, int8 coarseTune, int8 fineTune);

// Timer event: fires onTimer callback for the given timer slot (0-3)
static HiseEvent createTimerEvent(uint8 timerIndex, int offset);
```

For VolumeFade and PitchFade:
- The fade time is stored in the pitch wheel field (`getFadeTime()` / `setFadeTime()`)
- The EventID identifies which voice(s) to target

---

## 10. midi_logic Namespace -- MIDI-to-Modulation Processors

The `control::midi<MidiType>` node converts MIDI events to modulation signals.
The MidiType template argument implements the conversion logic via:

```cpp
bool getMidiValue(HiseEvent& e, double& value);
// Returns true if the event should trigger a modulation update.
// Sets value to 0.0..1.0 (normalized).
```

### Built-in midi_logic Types

| Type | Triggers On | Output (0..1) |
|------|-------------|---------------|
| `gate` | NoteOn and NoteOff | 1.0 for note-on, 0.0 for note-off |
| `velocity` | NoteOn only | velocity / 127.0 |
| `notenumber` | NoteOn only | noteNumberIncludingTranspose / 127.0 |
| `frequency` | NoteOn only | getFrequency() / 20000.0 |
| `random` | NoteOn only | Random 0.0..1.0 per note |

All types are templated with an unused `int` parameter (for mode template
compatibility). The `frequency` type has `IsProcessingHiseEvent() = true` as
a static constexpr, which is also used by duplilogic types that inherit from it
(harmonics, nyquist, fixed).

---

## 11. Key Patterns for Node Enrichment

### When documenting nodes with IsProcessingHiseEvent=true

1. **What events does it respond to?** Check the handleHiseEvent implementation
   for which `is*()` checks are used (isNoteOn, isController, isPitchWheel, etc.)

2. **Does it modify events?** The event is passed by reference. If the node calls
   `ignoreEvent(true)` or changes properties, downstream nodes are affected
   (in chain containers).

3. **Does it produce modulation from events?** Look for `ModValue` usage and
   `handleModulation()` -- the midi node pattern.

4. **Is it polyphonic?** If the node extends `polyphonic_base`, events are
   routed per-voice through VoiceDataStack. The node's handleHiseEvent runs
   with the correct voice context already set.

5. **Is it wrapped in wrap::event?** If so, the node gets sample-accurate
   event processing -- audio is split at event timestamps.

### Common handleHiseEvent Patterns

**Envelope gate (NoteOn/NoteOff):**
```cpp
void handleHiseEvent(HiseEvent& e)
{
    if (e.isNoteOn())
        // start envelope, store velocity/frequency
    else if (e.isNoteOff())
        // trigger release phase
}
```

**MIDI-to-modulation (normalized output):**
```cpp
void handleHiseEvent(HiseEvent& e)
{
    double v;
    if (midiProcessor.getMidiValue(e, v))
        modValue.setModValueIfChanged(v);
}
```

**Event passthrough (wrapper nodes):**
```cpp
void handleHiseEvent(HiseEvent& e) { obj.handleHiseEvent(e); }
// or: SN_DEFAULT_HANDLE_EVENT(T)
```
