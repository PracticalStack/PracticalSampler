## dump

**Examples:**

```javascript:debug-event-list-midi-conversion
// Title: Debugging event list contents during MIDI conversion
// Context: When processing MIDI event lists (e.g., remapping note numbers
// or adjusting velocities), dump() provides a quick way to inspect each
// event's state before and after modification.

const var mp = Synth.getMidiPlayer("Player1");
var eventList = mp.getEventList();

for (e in eventList)
{
    if (e.isNoteOn())
    {
        Console.print(e.dump());
        // Output: "Type: NoteOn, Channel: 1, Number: 36, Value: 100, EventId: 0, Timestamp: 480, "

        e.setNoteNumber(60);
        e.setVelocity(80);

        Console.print(e.dump());
        // Output: "Type: NoteOn, Channel: 1, Number: 60, Value: 80, EventId: 0, Timestamp: 480, "
    }
}
```
```json:testMetadata:debug-event-list-midi-conversion
{
  "testable": false,
  "skipReason": "Requires MidiPlayer module (Synth.getMidiPlayer) with loaded MIDI content"
}
```

For pitch wheel events the output format differs - Number, Value, and EventId are replaced with a single 14-bit `Value` field: `"Type: PitchBend, Channel: 1, Value: 8192, Timestamp: 0, "`.
