## getNoteRectangleList

**Examples:**

```javascript:piano-roll-paint-routine
// Title: Drawing a piano roll in a ScriptPanel paint routine
// Title: Drawing a piano roll in a ScriptPanel paint routine
// Context: Use getNoteRectangleList() inside setPaintRoutine() to render
// all notes from the current sequence, scaled to the panel bounds

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setUseTimestampInTicks(true);
mp.create(1, 4, 1);
var setupNotes = [];
var on = Engine.createMessageHolder();
var off = Engine.createMessageHolder();
on.setType(on.NoteOn);
off.setType(on.NoteOff);
on.setNoteNumber(64);
off.setNoteNumber(64);
on.setVelocity(100);
off.setVelocity(0);
on.setChannel(1);
off.setChannel(1);
on.setTimestamp(0);
off.setTimestamp(480);
setupNotes.push(on);
setupNotes.push(off);
mp.flushMessageList(setupNotes);
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
const var PianoRoll = Content.addPanel("PianoRoll", 0, 0);

mp.connectToPanel(PianoRoll);

PianoRoll.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);

    var notes = mp.getNoteRectangleList([0, 0, this.getWidth(), this.getHeight()]);

    // Store for verification
    this.data.noteCount = notes.length;

    if (notes.length == 0)
        return;

    g.setColour(0xFFAADDFF);

    for (r in notes)
        g.fillRect(r);

    // Draw playback cursor
    var x = mp.getPlaybackPosition() * this.getWidth();
    g.setColour(Colours.white);
    g.fillRect([x, 0, 2, this.getHeight()]);
});
```
```json:testMetadata:piano-roll-paint-routine
{
  "testable": true,
  "verifyScript": {
    "type": "REPL",
    "expression": "PianoRoll.data.noteCount > 0",
    "value": true
  }
}
```


Each rectangle in the returned array is `[x, y, width, height]` where x/width represent normalised time position and y/height represent note number (note 127 at top, note 0 at bottom). The rectangles are already scaled to the target bounds passed as the argument.
