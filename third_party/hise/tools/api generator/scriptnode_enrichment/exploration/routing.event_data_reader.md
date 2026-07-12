# routing.event_data_reader - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/RoutingNodes.h:407`
**Base class:** `mothernode` + `polyphonic_base`
**Classification:** control_source

## Signal Path

No audio processing (`SN_EMPTY_PROCESS`, `SN_EMPTY_PROCESS_FRAME`). Reads a value from the per-event AdditionalEventStorage and outputs it as a modulation signal.

Two modes based on the Static parameter:

**Dynamic mode (Static=Off):** `handleModulation()` calls `eventStorage->changed(currentEventId.get(), dataSlot, value)` on every modulation tick. This continuously polls the storage for the current voice's event ID at the configured slot index. Returns true if a valid value exists (event ID is found in storage).

**Static mode (Static=On):** On note-on, reads the value once from storage via `eventStorage->getValue()` and stores it in a local `ModValue staticValue`. Subsequent `handleModulation()` calls consume from this local ModValue. The value is fixed at note-on time and does not update if the storage changes later.

## Gap Answers

### event-data-storage-mechanism: What is the 'event ID storage of the global routing manager'? Is it a per-event-ID key-value store with 16 slots per event? How is data associated with a specific event ID?

The `AdditionalEventStorage` (in MiscToolClasses.h) is a fixed-size 2D array: `std::array<std::array<std::pair<uint16, double>, NumDataSlots>, NumEventSlots>` where `NumEventSlots = 1024` and `NumDataSlots = 16`. It is a hash-table-like structure keyed by event ID:

- `setValue(eventId, slotIndex, newValue)`: computes `i1 = eventId & (NumEventSlots-1)` (hash), stores `{eventId, newValue}` at `data[i1][slotIndex]`.
- `getValue(eventId, slotIndex)`: looks up `data[i1][slotIndex]`, verifies the stored eventId matches (collision check), returns `{true, value}` if match, `{false, 0.0}` if mismatch or eventId==0.

So each event ID can have up to 16 independent data slots (0-15). The storage persists until overwritten by another event ID that hashes to the same slot (1024 buckets with modulo hashing).

### read-trigger: When does the reader read the data -- on note-on events (handleHiseEvent), on every process() call, or triggered by some other mechanism?

Depends on the Static parameter:

- **Static=Off (dynamic):** The read happens in `handleModulation(double& value)`, which is called by the modulation system on every audio callback tick. It calls `eventStorage->changed(currentEventId.get(), dataSlot, value)` which returns true if a valid value exists for the current event ID.

- **Static=On:** The read happens once in `handleHiseEvent()` on note-on. The value is captured into a local `ModValue staticValue` via `staticValue.setModValue(v.second)`. Subsequent `handleModulation()` calls consume from this ModValue.

In both modes, `handleHiseEvent()` stores the event ID on note-on: `currentEventId.get() = e.getEventId()`.

### static-parameter-meaning: The Static parameter (0/1 toggle with converter) -- does it fix the SlotIndex at compile time for C++ export? Or does it change the reading behaviour?

It changes the reading behaviour at runtime. `setStatic(double newValue)` sets `isStatic = newValue > 0.5`. When Static=On, the value is read once at note-on time and cached in a local ModValue. When Static=Off, the value is read continuously from the event storage on each modulation tick. The parameter value names are "Off" and "On" (set via `setParameterValueNames`). This has nothing to do with compile-time fixing of SlotIndex.

### modulation-output-value: The node has ModulationTargets (modulation output). What value does it output -- the raw stored data, or a normalised 0..1 version?

The node declares `static constexpr bool isNormalisedModulation() { return true; }`, so the modulation output is treated as normalised 0..1 by the connection system. The raw value from storage is passed through directly -- the node itself does not clamp or scale. The writer (event_data_writer) stores whatever double value it receives, so the actual range depends on what was written. If values outside 0..1 are stored, they will be passed through but the connection system will apply target range conversion assuming 0..1 input.

### polyphonic-event-association: With IsPolyphonic true, does each voice read data associated with its own note-on event ID? How is the per-voice event ID resolved?

Yes, per-voice. The node has `PolyData<uint16, NV> currentEventId` -- each voice stores its own event ID. In `handleHiseEvent()`, when a note-on arrives, `currentEventId.get()` (which returns the current voice's slot via PolyHandler) is set to `e.getEventId()`. In `handleModulation()`, `currentEventId.get()` reads the current voice's stored event ID to look up the correct value in storage. Since `polyphonic_base` is constructed with `addProcessEventFlag=true`, events are routed per-voice through VoiceDataStack.

## Parameters

- **SlotIndex** (0-16, step 1, default 0): Selects which of the 16 data slots to read for the current event ID. Stored as `uint8 dataSlot`, clamped via `jlimit`.
- **Static** (0/1 toggle, default 0, names: "Off"/"On"): When On, reads value once at note-on and caches it. When Off, reads continuously from storage.

## Polyphonic Behaviour

- `PolyData<uint16, NV> currentEventId` -- stores the note-on event ID per voice
- `isPolyphonic()` returns `NV > 1`
- Constructor: `polyphonic_base(getStaticId(), true)` -- registers IsPolyphonic and IsProcessingHiseEvent
- Each voice independently tracks its event ID and reads its own data from storage

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

- The `CheckClass` template parameter (defaults to `NoCheck`) allows validation hooks in `initialise()` and `prepare()`. The default `NoCheck` does nothing.
- `prepare()` obtains the `AdditionalEventStorage` pointer via `ps.voiceIndex->getTempoSyncer()->additionalEventStorage`. If null, throws `Error::NoGlobalManager`.
- The `ModValue staticValue` member is shared across all voices (not per-voice). In static mode, each voice's note-on overwrites the same ModValue. This means in polyphonic static mode, the value updates each time any voice starts.
