<!-- Diagram triage:
  - ignoreEvent timing (artificial note-off reinsert): CUT (describes internal EventIdHandler mechanics stripped from user-facing prose)
-->

# Message

Message provides read and write access to the current MIDI event inside `onNoteOn`, `onNoteOff`, and `onController` callbacks. It is a globally available singleton - not created by user code - whose internal pointer is only valid during callback execution.

The class covers four areas of MIDI event handling:

1. **Basic event data** - note number, velocity, channel, controller number and value, timestamps.
2. **Pitch and level adjustment** - transpose, coarse detune, fine detune, per-event gain.
3. **Event routing** - ignoring events, delaying events, forwarding to MIDI output, storing events for later use.
4. **Artificial event system** - converting events to script-owned copies with stable event IDs for per-voice operations like pitch fades and volume fades.

All setter methods modify the event in-place in the audio buffer, affecting every downstream processor. Non-note events such as pitch bend, aftertouch, and program change all route through the `onController` callback. Pitch bend appears as virtual CC number 128 (`Message.PITCH_BEND_CC`) and aftertouch as 129 (`Message.AFTERTOUC_CC`), allowing uniform handling alongside standard CCs.

| Event Type | Callback | Controller Number |
|---|---|---|
| NoteOn | `onNoteOn` | N/A |
| NoteOff | `onNoteOff` | N/A |
| Controller (CC) | `onController` | 0-127 (actual CC) |
| PitchBend | `onController` | 128 |
| Aftertouch | `onController` | 129 |
| ProgramChange | `onController` | N/A (use `isProgramChange()`) |
| AllNotesOff | `setAllNotesOffCallback` handler | N/A |

> [!Tip:Setters need mutable callback context] Setter methods require a mutable callback context (a `JavascriptMidiProcessor` callback). Voice start modulators receive events as read-only and cannot call setters. To persist event data beyond callback scope, use `Message.store()` to copy into a `MessageHolder`.

## Common Mistakes

- **Match getter to event type**
  **Wrong:** Calling `Message.getNoteNumber()` inside `onController`.
  **Right:** Use `Message.getControllerNumber()` and `Message.getControllerValue()` for controller events.
  *Note getters require a NoteOn or NoteOff event. Calling them on controller events triggers a script error.*

- **Pair transpose with coarse detune**
  **Wrong:** `Message.setTransposeAmount(-5); Message.setCoarseDetune(0);`
  **Right:** `Message.setTransposeAmount(-5); Message.setCoarseDetune(5);`
  *Coarse detune must cancel the transpose to keep the audible pitch unchanged. Without it, the note sounds 5 semitones lower.*

- **Store events with Message.store**
  **Wrong:** Reading `Message` properties outside a MIDI callback.
  **Right:** Use `Message.store()` to copy into a `MessageHolder` created via `Engine.createMessageHolder()`.
  *The Message object's internal pointer is only valid during callback execution. Accessing it outside callbacks returns errors.*

- **Check event type before normalising**
  **Wrong:** Normalising `getControllerValue()` by dividing by 127 for all event types.
  **Right:** Check `getControllerNumber()` first - pitch bend returns 0-16383 (14-bit), not 0-127.
  *Dividing a pitch bend value by 127 produces values up to ~129 instead of the expected 0.0-1.0 range.*

- **Only delay artificial notes**
  **Wrong:** Applying `delayEvent()` to all notes unconditionally for humanisation.
  **Right:** Guard with `if (Message.isArtificial())` to only delay sequencer-generated notes.
  *Delaying live MIDI input adds unwanted latency to the player's performance.*
