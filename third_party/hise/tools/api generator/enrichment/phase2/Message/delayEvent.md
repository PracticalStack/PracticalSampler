## delayEvent

**Examples:**

```javascript:humanizing-sequencer-notes
// Title: Humanizing sequencer-generated notes with random timing slop
// Context: Drum sequencers generate perfectly quantized notes. Adding
// a small random delay to artificial (sequencer) events creates a
// more human feel without affecting live MIDI input.

const var slopKnob = Content.addKnob("SlopAmount", 10, 10);
slopKnob.setRange(0, 1, 0.01);

const var MAX_DELAY_SEC = 0.05; // 50ms maximum slop

function onNoteOn()
{
    if (Message.isArtificial() && slopKnob.getValue() > 0)
    {
        // Skewed random: Math.pow with exponent > 1 biases toward shorter delays
        local delaySamples = parseInt(slopKnob.getValue() *
            Engine.getSampleRate() * MAX_DELAY_SEC *
            Math.pow(Math.random(), 1.5));

        Message.delayEvent(delaySamples);
    }
}
```
```json:testMetadata:humanizing-sequencer-notes
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

```javascript:delay-for-controller-ordering
// Title: Delaying a note to ensure a preceding controller arrives first
// Context: When remapping notes to different pitches using pitch bend,
// the controller message must be processed before the note. Delaying
// the note by 1 sample guarantees ordering within the same buffer.

function onNoteOn()
{
    local noteNumber = Message.getNoteNumber();

    // Remap notes in range 60-84 using pitch bend for continuous tuning
    if (noteNumber >= 60 && noteNumber <= 84)
    {
        local semitones = noteNumber - 72;
        local pitchValue = Math.min(16383, 8192 + parseInt(semitones / 12.0 * 8192));

        // Send pitch bend first
        Synth.sendController(Message.PITCH_BEND_CC, pitchValue);

        // Delay note by 1 sample so pitch bend arrives first
        Message.delayEvent(1);
        Message.setTransposeAmount(-(noteNumber - 36));
    }
}
```
```json:testMetadata:delay-for-controller-ordering
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

**Pitfalls:**
- When using `delayEvent()` for humanization, guard it with `Message.isArtificial()` to avoid adding latency to live MIDI input. Live-played notes should pass through without delay.
