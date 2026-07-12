## isNoteOn

**Examples:**

```javascript:filter-midiplayer-event-list
// Title: Filtering a MidiPlayer event list for note-on events
// Context: When processing a MIDI sequence from MidiPlayer.getEventList(),
// use isNoteOn() to filter the array before modifying note properties.

const var mp = Synth.getMidiPlayer("Player1");
mp.setUseTimestampInTicks(true);

var eventList = mp.getEventList();

// Filter to only NoteOn events for analysis or modification
var noteOnEvents = eventList.filter(function(e)
{
    return e.isNoteOn();
});

// Remap note numbers using a conversion table
const var NOTE_MAP = {36: 60, 37: 62, 38: 64, 39: 65};

for (e in eventList)
{
    local nn = e.getNoteNumber();

    if (isDefined(NOTE_MAP[nn]))
    {
        if (e.isNoteOn() || e.isNoteOff())
            e.setNoteNumber(NOTE_MAP[nn]);
    }
}

mp.flushMessageList(eventList);
```
```json:testMetadata:filter-midiplayer-event-list
{
  "testable": false,
  "skipReason": "Requires MidiPlayer module (Synth.getMidiPlayer) with loaded MIDI content"
}
```

Note that NoteOn events with velocity 0 are still classified as NoteOn in HISE's event system - they are not automatically converted to NoteOff as in some MIDI implementations.
