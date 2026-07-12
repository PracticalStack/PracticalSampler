## isArtificial

**Examples:**

```javascript:selective-humanization-sequencer
// Title: Selective humanization for sequencer-generated notes only
// Context: A loop processor applies random timing slop to sequencer hits
// but leaves live MIDI input unmodified. Sequencer-generated notes
// (from Synth.addNoteOn or MidiPlayer) are flagged as artificial.

const var slopAmount = Content.addKnob("SlopAmount", 10, 10);
slopAmount.setRange(0, 1, 0.01);

const var slopEnabled = Content.addButton("EnableSlop", 10, 50);

const var MAX_DELAY_SEC = 0.05;

function onNoteOn()
{
    // Only humanize artificial (sequencer/arpeggiator) notes
    if (Message.isArtificial() && slopEnabled.getValue())
    {
        local maxSamples = Engine.getSampleRate() * MAX_DELAY_SEC;
        local delay = parseInt(slopAmount.getValue() * maxSamples * Math.random());
        Message.delayEvent(delay);
    }
}
```
```json:testMetadata:selective-humanization-sequencer
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

**Cross References:**
- `Message.makeArtificial`
- `Synth.addNoteOn`
