# routing.event_data_writer - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/RoutingNodes.h:518`
**Base class:** `mothernode` + `polyphonic_base`
**Classification:** utility

## Signal Path

No audio processing (`SN_EMPTY_PROCESS`, `SN_EMPTY_PROCESS_FRAME`). Writes a value to the per-event AdditionalEventStorage.

Two write triggers:

1. **On note-on** (`handleHiseEvent`): When a note-on event arrives, stores the current event ID and immediately writes the current value to storage: `eventStorage->setValue(eventId, dataSlot, value, dontSendNotification)`.

2. **On Value parameter change** (`setValue`): Iterates all voices via `for(auto& s: currentEventId)`, writes the current value for each voice's stored event ID: `eventStorage->setValue(s.first, dataSlot, newValue, dontSendNotification)`. This updates the stored value for all active voices.

## Gap Answers

### write-trigger: When does the writer write data -- on every process() call, on note-on events (handleHiseEvent), or whenever the Value parameter changes?

Both on note-on AND whenever the Value parameter changes. On note-on, `handleHiseEvent()` stores the event ID and writes the current cached value. On parameter change, `setValue()` writes the new value for ALL active voices' event IDs. The write is NOT on every process() call -- process() is empty.

### value-range-storage: The Value parameter has range 0-1. Is the stored value always 0..1, or can unnormalised values be stored if connected to an unnormalised source?

The storage accepts any double value -- `AdditionalEventStorage::setValue()` stores a raw `double` without clamping. The Value parameter's 0-1 range is the nominal input range, but if an unnormalised source sends values outside 0-1, those values will be stored as-is. The `event_data_reader` will then output whatever was stored.

### polyphonic-write-isolation: With IsPolyphonic true, does each voice write to storage keyed by its own event ID? Can voice A's write be read by voice B's event_data_reader?

Each voice writes keyed by its own event ID. The `currentEventId` is `PolyData<std::pair<uint16, double>, NV>` -- each voice stores its own `{eventId, value}` pair. On note-on, only the current voice's slot is updated with the new event ID.

However, voice A's data CAN be read by voice B's event_data_reader IF voice B happens to know voice A's event ID. In practice this would not normally happen since each reader stores its own voice's event ID. The isolation is by event ID, not by voice index -- it is the event ID matching that provides per-voice isolation.

### write-persistence: How long does the written data persist? Until the event ID is recycled (note-off + voice kill)? Or until explicitly overwritten?

Data persists until the storage slot is overwritten. The AdditionalEventStorage uses modulo hashing (`eventId & (NumEventSlots-1)`), so data persists until a different event ID with the same hash bucket writes to the same slot index. With 1024 event slots and uint16 event IDs (65536 range), a slot is overwritten when another event ID maps to the same bucket AND writes to the same data slot index. There is no explicit cleanup on note-off. In practice, the data lifetime extends well beyond the note's lifetime but is not guaranteed indefinitely.

## Parameters

- **SlotIndex** (0-16, step 1, default 0): Selects which of the 16 data slots to write for the current event ID. Stored as `uint8 dataSlot`, clamped via `jlimit`.
- **Value** (0..1, default 0): The value to write to event storage. Written on every parameter change for all active voices, and on note-on for the new voice.

## Polyphonic Behaviour

- `PolyData<std::pair<uint16, double>, NV> currentEventId` -- per-voice storage of `{eventId, lastWrittenValue}`
- `isPolyphonic()` returns `NV > 1`
- Constructor: `polyphonic_base(getStaticId(), true)` -- registers IsPolyphonic and IsProcessingHiseEvent
- `handleHiseEvent()`: on note-on, stores `{e.getEventId(), currentValue}` for the current voice and writes to storage
- `setValue()`: iterates ALL voices and writes the new value for each voice's stored event ID

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

- The `dontSendNotification` parameter in `eventStorage->setValue()` means the storage's LambdaBroadcaster is not triggered on writes from the audio thread. This is correct for audio-thread safety.
- Unlike event_data_reader, the writer has no Static parameter and no modulation output. It is purely a data sink.
- The `CheckClass` template parameter works identically to event_data_reader.
- `prepare()` obtains AdditionalEventStorage via `ps.voiceIndex->getTempoSyncer()->additionalEventStorage`. Throws `Error::NoGlobalManager` if null.
