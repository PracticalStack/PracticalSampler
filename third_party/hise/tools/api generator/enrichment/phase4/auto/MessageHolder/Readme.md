<!-- Diagram triage:
  - message-holder-lifecycle: RENDER (complex fan-in/fan-out topology with 4 sources and 4 sinks - not expressible in prose)
-->

# MessageHolder

MessageHolder is a persistent MIDI event container that stores event data independently of callback scope. Unlike `Message`, which references the live event during a MIDI callback and becomes invalid when the callback returns, MessageHolder owns a copy of the event data and persists as long as your script holds a reference.

This makes MessageHolder the right tool for several workflows:

- **Store and re-inject**: Capture an event with `Message.store()`, suppress the original, modify properties, and re-inject later via `Synth.addMessageFromHolder()`.
- **Sequence construction**: Build MIDI patterns from scratch by creating MessageHolder objects, setting their type, note, velocity, and timestamp, then flushing the list to a MidiPlayer.
- **Event list processing**: Read existing MIDI data from `MidiPlayer.getEventList()` or `File.loadAsMidiFile()`, iterate the returned MessageHolder array, modify events, and write them back.
- **Note tracking**: Combine with `UnorderedStack` in event-stack mode to maintain a live set of held notes for sustain pedal logic, chord detection, or sympathetic resonance.

You can create a MessageHolder with `Engine.createMessageHolder()`, or receive them from APIs that return event data. A newly created holder has type Empty and all fields zeroed - set the type before using it.

```js
const var mh = Engine.createMessageHolder();
```

| Type Constant | Value | Description |
|---|---|---|
| `Empty` | 0 | Default uninitialised state; rejected by `Synth.addMessageFromHolder()` |
| `NoteOn` | 1 | Note-on event |
| `NoteOff` | 2 | Note-off event |
| `Controller` | 3 | MIDI CC |
| `PitchBend` | 4 | Pitch wheel (14-bit value) |
| `Aftertouch` | 5 | Channel pressure or polyphonic aftertouch |
| `AllNotesOff` | 6 | Kills all active voices |
| `ProgramChange` | 13 | MIDI program change |

> [!Tip:No type guards on getters and setters] MessageHolder performs no type guards on getters and setters. Calling `getNoteNumber()` on a Controller event returns whatever is in the number byte - the class is a raw data container, so ensure you read and write fields appropriate for the event type. The only validation is on `setType()`, which rejects values outside 0-13.

## Common Mistakes

- **Set type before adding to synth**
  **Wrong:** `Synth.addMessageFromHolder(mh)` without setting the type first.
  **Right:** `mh.setType(mh.NoteOn); mh.setNoteNumber(60); Synth.addMessageFromHolder(mh);`
  *A default MessageHolder has type Empty. Passing it to `Synth.addMessageFromHolder()` triggers an "Event is empty" error.*

- **Clone or create new holders per event**
  **Wrong:** Reusing a single MessageHolder in a loop and pushing it to an array.
  **Right:** Create a new `Engine.createMessageHolder()` for each event, or use `clone()`.
  *MessageHolder is a reference-counted object. Pushing the same reference multiple times means all array slots point to the same event - every modification affects all "copies".*

- **Verify stored event type before injection**
  **Wrong:** Calling `Synth.addMessageFromHolder(holder)` after `Message.store(holder)` in `onNoteOff` without checking the stored event type.
  **Right:** Use `Message.store(holder)` which captures the correct type from the callback context, and verify the stored type matches your intent before re-injection.
  *When `Message.store()` runs in `onNoteOff`, the stored type is NoteOff. Re-injecting a NoteOff without a matching NoteOn event ID can produce orphaned voice state.*

- **Use proper timestamps for MidiPlayer events**
  **Wrong:** Setting timestamp to 0 for all events in a MidiPlayer sequence.
  **Right:** Use `setTimestamp()` with proper tick or sample offsets based on `MidiPlayer.setUseTimestampInTicks()`.
  *MidiPlayer uses timestamps to position events. All events at timestamp 0 fire simultaneously on the first beat.*
