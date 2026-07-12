# Event Data Modulator - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/EventDataModulator.h`, `hi_core/hi_modules/modulators/mods/EventDataModulator.cpp`
**Base class:** `VoiceStartModulator`

## Signal Path

noteOn HiseEvent -> extract eventId -> lookup AdditionalEventStorage[eventId][dataSlot] -> if found: return clamped value (0.0-1.0) -> if not found: return defaultValue -> output as voice start modulation value

The modulator is invoked via the VoiceStartModulator base class flow: `handleHiseEvent()` receives noteOn -> calls `calculateVoiceStartValue()` -> stores result in `unsavedValue` -> `startVoice()` copies `unsavedValue` into `voiceValues[voiceIndex]`.

## Gap Answers

### event-data-write-mechanism: How are event data slots written?

Event data is written via the `AdditionalEventStorage` struct (defined in `hi_tools/hi_tools/MiscToolClasses.h`). There are three write paths:

1. **Scripting API (primary):** `GlobalRoutingManager.setEventData(eventId, slotIndex, value)` calls `additionalEventStorage.setValue()`. This is the main user-facing write mechanism, accessed through `Engine.getGlobalRoutingManager()` in HiseScript.

2. **Scriptnode:** The `RoutingNodes.h` file shows scriptnode nodes that access `additionalEventStorage` via `PrepareSpecs`, allowing event data to be written from within the scriptnode graph.

3. **ComplexGroupManager:** The sampler's ComplexGroupManager reads event data slots (source type `EventData`) but this is a read path, not a write path.

The storage is a fixed 2D array: `data[NumEventSlots][NumDataSlots]` where NumEventSlots=1024, NumDataSlots=16. Event IDs are hashed via bitmask `eventId & (NumEventSlots - 1)`, creating a hash table with potential collisions for events more than 1024 apart.

### slot-index-range-discrepancy: SlotIndex range max is 16, but description says 0-15

This is a confirmed bug. There are 16 slots indexed 0-15 (`NumDataSlots = 16`). However:

- `setInternalAttribute` uses `jlimit<uint8>(0, NumDataSlots, ...)` which clamps to [0, 16], allowing index 16.
- The slider range in metadata is set to `InvertableParameterRange(0.0, 16.0, 1.0)`, also allowing 16.
- Inside `AdditionalEventStorage::getValue()`, the slot index is masked: `slotIndex & (NumDataSlots - 1)` = `slotIndex & 15`. Index 16 (binary 10000) masks to 0, silently aliasing to slot 0.

So the parameter allows setting slot 16, but it silently reads slot 0. Valid indices are 0-15 (16 slots total). The range max should be 15 (or the description should say 0-16 with 17 slots, but the storage only has 16).

### voice-start-read-timing: Does the value remain constant for the entire voice lifetime?

Yes. As a VoiceStartModulator, the value is sampled once during `handleHiseEvent()` when a noteOn arrives. The result is stored in `unsavedValue`, then copied to `voiceValues[voiceIndex]` in `startVoice()`. There is no per-block recalculation. The value is constant for the voice lifetime. If the event data slot is written after note-on, the voice does not see the update.

For live-updating behavior, users should use EventDataEnvelope instead (which re-reads the slot every `calculateBlock` call).

### default-value-detection: How does the modulator detect an unwritten slot?

The detection mechanism in `AdditionalEventStorage::getValue()` works as follows:

1. If `eventId == 0`, returns `{false, 0.0}` (event ID 0 is invalid).
2. The storage element at `data[hashedId][slotIndex]` stores a pair: `{storedEventId, value}`.
3. If `storedEventId == eventId`, the slot has been written for this event -> returns `{true, value}`.
4. If `storedEventId != eventId`, the slot was never written for this event (or was overwritten by a different event due to hash collision) -> returns `{false, 0.0}`.

In `calculateVoiceStartValue()`, if the pair's `first` is true, the stored value is used. Otherwise, `defaultValue` is returned. This means: writing 0.0 to a slot returns `{true, 0.0}`, so the modulator correctly uses 0.0, NOT the defaultValue. The distinction between "not written" and "written with zero" is correctly handled.

### event-data-scope: Is event data per-note, per-channel, or global?

Event data is per-event-ID. Each HiseEvent has a unique 16-bit event ID assigned by HISE's event system. The storage is keyed by event ID, so different simultaneous notes have independent data in the same slot index. This makes it truly per-voice in polyphonic contexts.

However, event IDs are recycled (hashed to 1024 slots via bitmask), so events with IDs that are exactly 1024 apart will collide. In practice this is not an issue as event IDs increment sequentially and 1024 simultaneous events is extremely unlikely.

## Processing Chain Detail

1. **Event ID extraction** (per-voice, negligible CPU): Gets the event ID from the incoming HiseEvent.
2. **Storage lookup** (per-voice, negligible CPU): Hash-table lookup in `AdditionalEventStorage` using `eventId & 1023` and `slotIndex & 15`. Returns pair of (found, value).
3. **Default fallback** (per-voice, negligible CPU): If slot was not found for this event ID, returns `defaultValue` parameter.
4. **Value clamping** (per-voice, negligible CPU): Clamps result to [0.0, 1.0] via `jlimit`.

## Modulation Points

None. This module has no modulation chains. The two parameters (SlotIndex, DefaultValue) are static -- not modulatable.

## Conditional Behavior

The only conditional is the found/not-found check on the storage lookup:
- **Slot written for this event ID:** Returns the stored value, clamped to [0.0, 1.0].
- **Slot not written (or event ID is 0, or hash collision):** Returns `defaultValue`.

There is also a null-check on the GlobalRoutingManager: if no GlobalRoutingManager exists, an error message is logged and `defaultValue` is returned.

## Vestigial / Notable

- The Doxygen comment in the header file describes this as "A constant Modulator which calculates a random value at the voice start" with mention of a "look up table." This is a copy-paste artifact from RandomModulator. The EventDataModulator has no table and no randomness.
- The `additionalEventStorage` member pointer is initialized in the constructor from the GlobalRoutingManager. However, `calculateVoiceStartValue()` does NOT use this cached pointer -- it re-fetches the GlobalRoutingManager from MainController on every call. The cached pointer is unused (vestigial in EventDataModulator, but used in EventDataEnvelope).

## CPU Assessment

Overall: **negligible**. A single hash-table lookup per voice start with no per-sample processing, no allocations, no math beyond a clamp. This is one of the lightest possible modulators.

## UI Components

The editor is `EventDataEditor` (a custom `ProcessorEditorBody`, not a FloatingTile). Contains two `HiSlider` controls: SlotIndex (discrete) and DefaultValue (normalized percentage). No FloatingTile content types.

## Notes

- EventDataEnvelope (also defined in the same files) is the time-variant sibling. It re-reads the event data slot every `calculateBlock()` call with smoothing, allowing the modulation value to change during the voice lifetime. The EventDataModulator is the sample-and-hold version.
- The GlobalRoutingManager acts as a bridge: scripts write data via `setEventData()`, and modulators read it. This enables cross-module communication keyed by event identity.
- The `additionalEventStorage` member pointer in EventDataModulator is set in the constructor but never used in `calculateVoiceStartValue()`, which instead re-fetches via `getMainController()->getGlobalRoutingManager()`. This is inconsistent with EventDataEnvelope which uses its cached pointer.
