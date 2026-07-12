## setValue

**Examples:**

```javascript:track-velocity-for-release-triggers
// Title: Track velocity per note for release sample triggering
// Context: Sampler plugins need to recall each note's velocity when the
// key is released to play release samples at the matching dynamic level.

const var velocities = Engine.createMidiList();

// In onNoteOn:
velocities.setValue(Message.getNoteNumber(), Message.getVelocity());

// In onNoteOff:
Synth.playNote(Message.getNoteNumber(), velocities.getValue(Message.getNoteNumber()));
```
```json:testMetadata:track-velocity-for-release-triggers
{
  "testable": false,
  "skipReason": "Requires MIDI note-on/note-off events to populate Message.getNoteNumber() and Message.getVelocity()"
}
```

```javascript:note-to-channel-routing-table
// Title: Build a MIDI note-to-channel routing table
// Context: A multi-channel instrument maps each MIDI note to a channel
// index. Unassigned notes remain at -1 (the default after clear).

const var NUM_CHANNELS = 4;
const var channelKeys = [36, 38, 42, 46]; // kick, snare, hihat, open hihat

const var noteToChannel = Engine.createMidiList();

// Populate the routing table
for (i = 0; i < NUM_CHANNELS; i++)
    noteToChannel.setValue(channelKeys[i], i);

// Look up channel for incoming note
Console.print(noteToChannel.getValue(36));  // 0 (first channel)
Console.print(noteToChannel.getValue(60));  // -1 (unassigned)
```
```json:testMetadata:note-to-channel-routing-table
{
  "testable": true,
  "verifyScript": {
    "type": "log-output",
    "values": ["0", "-1"]
  }
}
```
