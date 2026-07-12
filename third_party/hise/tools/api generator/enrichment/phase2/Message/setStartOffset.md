## setStartOffset

**Examples:**

```javascript:per-note-phase-offset
// Title: Per-note phase offset for oscillator start position
// Context: An additive synthesis instrument uses setStartOffset to start
// each harmonic oscillator at a different phase position per note,
// creating organic key-click and attack variation. The phase angle
// is converted from radians to a sample offset within a single
// wavetable cycle.

const var CYCLE_LENGTH = 2048; // Samples per wavetable cycle

// Pre-computed phase offsets per MIDI note (radians)
const var phaseTable = [];

for (i = 0; i < 128; i++)
    phaseTable.push(Math.sin(i * 1.7) * Math.PI); // Pseudo-random distribution

function onNoteOn()
{
    local note = Message.getNoteNumber();

    // Convert phase angle (radians) to sample offset within one cycle
    local phaseDeg = Math.toDegrees(phaseTable[note] + (2 * Math.PI));
    local offset = parseInt(phaseDeg / 360 * CYCLE_LENGTH);

    Message.setStartOffset(offset);
}
```
```json:testMetadata:per-note-phase-offset
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

**Pitfalls:**
- The start offset is a `uint16` with a maximum of 65535 samples (about 1.36 seconds at 48kHz). For longer skip amounts, consider loading a different sample rather than using start offset.
- Start offset tells the sound generator where to begin playback within the sample. It does NOT delay the event - use `delayEvent()` for timing delays.
