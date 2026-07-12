## filter

**Examples:**

```javascript:filter-midi-by-range
// Title: Filter MIDI events by time range and type
// Context: A step sequencer reads a MidiPlayer's event list, then
// filters to find note-on events for a specific note within a
// time window. Each sequencer step queries its own time range.

const var mp = Synth.getMidiPlayer("Player1");
mp.setUseTimestampInTicks(true);

const var events = mp.getEventList();
const var ticksPerBar = mp.getTicksPerQuarter() * 4;

// Filter note-on events for a specific note within a time range
const var NOTE = 36;
const var noteOns = events.filter(function(e)
{
    return e.getTimestamp() < ticksPerBar &&
           e.isNoteOn() &&
           e.getNoteNumber() == NOTE;
});

Console.print("Found " + noteOns.length + " hits for note " + NOTE);

// Filter controller events by CC number
const var CHANCE_CC = 20;
const var chanceEvents = events.filter(function(e)
{
    return e.isController() &&
           e.getControllerNumber() == CHANCE_CC;
});
```
```json:testMetadata:filter-midi-by-range
{
  "testable": false,
  "skipReason": "Requires a MidiPlayer module with loaded MIDI content"
}
```

```javascript:filter-captured-variable
// Title: Filter with captured variable
// Context: When the filter condition depends on a local variable,
// use the [capture] syntax to bring it into scope.

const var mp = Synth.getMidiPlayer("Player1");
const var events = mp.getEventList();

inline function getControllersForCC(list, ccNumber)
{
    return list.filter(function [ccNumber](e)
    {
        return e.isController() &&
               e.getControllerNumber() == ccNumber;
    });
}

const var modWheelEvents = getControllersForCC(events, 1);
```
```json:testMetadata:filter-captured-variable
{
  "testable": false,
  "skipReason": "Requires a MidiPlayer module with loaded MIDI content"
}
```

**Pitfalls:**
- When filtering inside a loop (e.g., per sequencer step), each call allocates a new result array. For tight loops, consider a manual `for` loop that collects results into a pre-allocated array.
