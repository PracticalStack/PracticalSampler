# MessageHolder -- Project Context

## Project Context

### Real-World Use Cases
- **Release sample triggering**: A piano or sampler plugin stores incoming NoteOn events into MessageHolder objects (via `Message.store()`), ignores the original event, then later re-injects a modified version via `Synth.addMessageFromHolder()` on NoteOff. The stored event carries the original velocity and note number, while the script adds gain adjustments and selects sample groups based on hold duration. This pattern gives finer control over release sample selection than the built-in release trigger mode.
- **MIDI sequence construction**: A drum machine or sequencer builds MIDI patterns programmatically by creating MessageHolder pairs (NoteOn + NoteOff) with explicit timestamps in ticks, then flushing the entire list to a MidiPlayer via `flushMessageList()`. This enables slider-pack-driven step sequencing where UI changes regenerate the entire MIDI sequence on the fly.
- **Event list manipulation**: A plugin that exports audio or MIDI reads an existing sequence via `MidiPlayer.getEventList()` (which returns MessageHolder arrays), modifies individual events (remapping note numbers, adjusting timestamps for tempo changes), and either flushes them back or writes them to a MIDI file via `File.writeMidiFile()`.
- **Note tracking with UnorderedStack**: A sampler uses MessageHolder + UnorderedStack in event-stack mode to maintain a live set of held notes. `Message.store()` pushes events, `removeIfEqual()` removes matching NoteOffs, and `storeEvent()` reads back events for deferred re-injection. This is the foundation for sustain pedal, sympathetic resonance, and chord-aware processing.

### Complexity Tiers
1. **Store and re-inject** (most common): `Engine.createMessageHolder()`, `Message.store()`, `setGain()`, `setVelocity()`, `Synth.addMessageFromHolder()`. A MIDI processor intercepts events, stores them, and re-injects modified copies later.
2. **Sequence construction**: `setType()`, `setNoteNumber()`, `setVelocity()`, `setChannel()`, `setTimestamp()`, `setControllerNumber()`, `setControllerValue()`. Builds MIDI patterns from scratch for MidiPlayer or offline rendering.
3. **Event list processing**: `getEventList()` returns MessageHolder arrays, then iterate with `isNoteOn()`, `getNoteNumber()`, `getTimestamp()`, `setTimestamp()`, `setNoteNumber()`, `dump()`. Manipulates existing MIDI data for export, conversion, or analysis.

### Practical Defaults
- Always set the type before any other field. A default-constructed MessageHolder has type Empty, which `Synth.addMessageFromHolder()` rejects with an error.
- Use `Message.store(holder)` rather than manually copying each field. It captures all event data (type, channel, note, velocity, event ID, timestamp) in one call.
- When constructing NoteOn/NoteOff pairs for MidiPlayer sequences, create separate MessageHolder objects for each event. Reusing a single holder and pushing it to an array stores a reference, not a copy - all entries in the array will reflect the last modification.
- For note tracking across callbacks, combine MessageHolder with `UnorderedStack.setIsEventStack(true, stack.EventId)`. The EventId compare mode matches NoteOn/NoteOff pairs automatically.
- Set channel to 1 (not 0) for standard MIDI. A newly created MessageHolder has channel 0, which is outside the standard MIDI 1-16 range.

### Integration Patterns
- `Message.store(holder)` -> `Message.ignoreEvent(true)` -> later `Synth.addMessageFromHolder(holder)` - The "intercept, defer, re-inject" pattern for release triggers, arpeggiators, and note scheduling.
- `Engine.createMessageHolder()` -> `setType/setNoteNumber/setTimestamp` -> `array.push(holder)` -> `MidiPlayer.flushMessageList(array)` - Programmatic sequence construction for step sequencers.
- `MidiPlayer.getEventList()` -> iterate MessageHolder array -> modify events -> `MidiPlayer.flushMessageList(list)` or `File.writeMidiFile(list)` - Event list round-trip for MIDI export and conversion.
- `UnorderedStack.setIsEventStack(true)` -> `stack.insert(holder)` on NoteOn -> `stack.removeIfEqual(holder)` on NoteOff -> `stack.storeEvent(i, holder)` on pedal-off - Active note tracking with sustain pedal support.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Reusing a single MessageHolder in a loop and pushing it to an array | Creating a new `Engine.createMessageHolder()` for each event, or using `holder.clone()` | MessageHolder is a reference-counted object. Pushing the same reference multiple times means all array slots point to the same event. Each modification affects every "copy" in the array. |
| Calling `Synth.addMessageFromHolder(holder)` without checking `holder.isNoteOn()` type first in a release trigger script | Using `Message.store(holder)` which captures the correct event type from the callback context | When `Message.store()` is called in `onNoteOff`, the stored event type is NoteOff. Re-injecting a NoteOff without a matching NoteOn event ID can produce orphaned voice state. Ensure the stored event has the correct type for your use case. |
| Setting timestamp to 0 for all events in a MidiPlayer sequence | Using `setTimestamp()` with proper tick or sample offsets based on `MidiPlayer.setUseTimestampInTicks()` | MidiPlayer uses timestamps to position events in the sequence. All events at timestamp 0 fire simultaneously on the first beat, regardless of their intended rhythmic position. |
