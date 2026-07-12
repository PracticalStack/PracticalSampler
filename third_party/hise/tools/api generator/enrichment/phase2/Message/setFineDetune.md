## setFineDetune

**Examples:**

```javascript:pseudo-random-detuning
// Title: Pseudo-random detuning for sample redistribution
// Context: When a sampler has limited samples in the upper register,
// multiple notes can share the same sample with slight per-note detuning
// to avoid the "machine gun" effect. A deterministic pseudo-random table
// ensures consistent results across plays.

const var SHARED_RANGE_LOW = 50;
const var SHARED_RANGE_HIGH = 90;

// Build a deterministic pseudo-random detune table
const var detuneTable = [];

for (i = 0; i < 128; i++)
    detuneTable.push(parseInt(Math.sin(i * 925512.2934) * 100.0));

function onNoteOn()
{
    local note = Message.getNoteNumber();

    if (note >= SHARED_RANGE_LOW && note <= SHARED_RANGE_HIGH)
    {
        // Map multiple notes to the same sample via transpose
        local sampleNote = SHARED_RANGE_LOW + (note - SHARED_RANGE_LOW) % 2;

        Message.setTransposeAmount(sampleNote - note);

        // Add pseudo-random fine detuning for natural variation
        Message.setFineDetune(detuneTable[note]);
    }
}
```
```json:testMetadata:pseudo-random-detuning
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```
