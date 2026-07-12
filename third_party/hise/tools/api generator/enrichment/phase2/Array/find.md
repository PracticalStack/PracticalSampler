## find

**Examples:**

```javascript:find-midi-event-by-id
// Title: Find a matching MIDI event by event ID
// Context: A sequencer finds the note-off event that pairs with
// a specific note-on, using the event ID to match them.

const var mp = Synth.getMidiPlayer("Player1");
const var events = mp.getEventList();
const var noteOnId = 42;

const var matchingNoteOff = events.find(function(e)
{
    return e.getEventId() == noteOnId && e.isNoteOff();
});

if(isDefined(matchingNoteOff))
{
    local offTimestamp = matchingNoteOff.getTimestamp();
    Console.print("Note-off at tick " + offTimestamp);
}
```
```json:testMetadata:find-midi-event-by-id
{
  "testable": false,
  "skipReason": "Requires a MidiPlayer module with loaded MIDI content"
}
```
