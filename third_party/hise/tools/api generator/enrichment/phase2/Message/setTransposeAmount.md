## setTransposeAmount

**Examples:**

```javascript:timbre-shifting
// Title: Timbre shifting without changing audible pitch
// Context: In a sampled instrument, shifting which sample is triggered
// changes the timbre (brighter/darker) without changing the heard pitch.
// setTransposeAmount selects a different sample zone, while setCoarseDetune
// with the opposite sign cancels the pitch change.

const var timbreKnob = Content.addKnob("Timbre", 10, 10);
timbreKnob.setRange(-12, 12, 1);

function onNoteOn()
{
    local amount = parseInt(timbreKnob.getValue());

    // Transpose selects a different sample zone
    Message.setTransposeAmount(-amount);

    // Coarse detune cancels the pitch shift so the note sounds at the original pitch
    Message.setCoarseDetune(amount);
}
```
```json:testMetadata:timbre-shifting
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

```javascript:velocity-aware-timbre-shift
// Title: Velocity-aware timbre shifting with range clamping
// Context: A piano plugin shifts timbre by a variable amount, clamping
// to the instrument's sample range so extreme values don't go out of bounds.

const var timbreKnob = Content.addKnob("Timbre", 10, 10);
timbreKnob.setRange(0, 4, 1);

const var NOTE_LOW = 21;   // Lowest sampled note (A0)
const var NOTE_HIGH = 108; // Highest sampled note (C8)

function onNoteOn()
{
    local amount = parseInt(timbreKnob.getValue()) * 2;

    // Clamp so the transposed note stays within the sample range
    local targetNote = Math.range(Message.getNoteNumber() - amount, NOTE_LOW, NOTE_HIGH);
    local actualShift = Message.getNoteNumber() - targetNote;

    Message.setTransposeAmount(-actualShift);
    Message.setCoarseDetune(actualShift);
}
```
```json:testMetadata:velocity-aware-timbre-shift
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

**Pitfalls:**
- Always pair `setTransposeAmount(-N)` with `setCoarseDetune(N)` for timbre shifting. If coarse detune is omitted, the note will sound at the transposed pitch rather than the original.
- Transpose is automatically copied from note-on to note-off by the EventIdHandler, so you only need to set it in `onNoteOn`. Coarse detune is NOT automatically copied -- but for timbre shifting this is usually fine since coarse detune only affects sample selection at voice start.
