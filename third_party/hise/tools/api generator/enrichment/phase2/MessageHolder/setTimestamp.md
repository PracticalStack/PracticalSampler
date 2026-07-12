## setTimestamp

**Examples:**

```javascript:timestamp-multi-track-export
// Title: Manipulating event timestamps for multi-track MIDI export
// Context: When exporting a multi-channel sequence to a single MIDI file,
// each track's events need their timestamps offset to account for tempo
// scaling and sequential stem placement.

const var mp = Synth.getMidiPlayer("Player1");
mp.setUseTimestampInTicks(true);

var events = [];
var trackList = mp.getEventList();
var stemOffset = 0;
var ticksPerQuarter = mp.getTicksPerQuarter();

// Remap timestamps relative to a stem offset position
for (event in trackList)
{
    local originalTick = event.getTimestamp();
    event.setTimestamp(stemOffset + originalTick);
    events.push(event);
}

// Next stem starts after this track's content
stemOffset += ticksPerQuarter * 8;
```
```json:testMetadata:timestamp-multi-track-export
{
  "testable": false,
  "skipReason": "Requires MidiPlayer module (Synth.getMidiPlayer) with loaded MIDI content"
}
```

When working with `MidiPlayer.getEventList()`, the timestamp format (ticks or samples) depends on `MidiPlayer.setUseTimestampInTicks()`. Always set the format before calling `getEventList()` and use the same format when modifying timestamps and flushing back.
