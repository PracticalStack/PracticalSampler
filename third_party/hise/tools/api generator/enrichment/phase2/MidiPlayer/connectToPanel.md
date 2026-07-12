## connectToPanel

**Examples:**

```javascript:piano-roll-visualization
// Title: Piano roll visualization with connected panel
// Context: Connect a MidiPlayer to a ScriptPanel for automatic repainting
// when the sequence changes, and draw notes using getNoteRectangleList()

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var mpIdx = builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");
const var PianoRoll = Content.addPanel("PianoRoll", 0, 0);

// Connect so the panel repaints when the sequence changes
mp.connectToPanel(PianoRoll);

PianoRoll.setPaintRoutine(function(g)
{
    g.fillAll(0xFF666666);

    var rList = mp.getNoteRectangleList([0, 0, this.getWidth(), this.getHeight()]);

    g.setColour(Colours.white);

    for (r in rList)
        g.fillRect(r);
});
```
```json:testMetadata:piano-roll-visualization
{
  "testable": false,
  "skipReason": "Paint routine output cannot be verified via REPL; visual-only example"
}
```

The panel automatically receives `repaint()` calls when the sequence data changes (file load, edit, clear). To also repaint during playback (for a moving position cursor), call `setRepaintOnPositionChange(true)`.
