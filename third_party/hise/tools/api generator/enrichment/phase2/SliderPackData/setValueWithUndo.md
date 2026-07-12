## setValueWithUndo

**Examples:**

```javascript:record-midi-to-steps
// Title: Recording MIDI input into a step sequencer with undo support
// Context: When a note is played during recording, the velocity and
// pitch offset are written to the current step position. Using
// setValueWithUndo() ensures the entire recording can be undone.

const var NUM_MODES = 2;
const var VELOCITY = 0;
const var PITCH = 1;
const var NUM_STEPS = 16;

// Create data packs for velocity and pitch
const var packs = [];
packs.push(Engine.createAndRegisterSliderPackData(0));
packs.push(Engine.createAndRegisterSliderPackData(1));

for (p in packs)
{
    p.setNumSliders(NUM_STEPS);
    p.setAllValues(0.0);
    p.setUsePreallocatedLength(NUM_STEPS);
}

// In onNoteOn: record velocity and pitch at the current step
inline function recordStep(step, velocity, pitchOffset)
{
    packs[VELOCITY].setValueWithUndo(step, velocity / 127.0);
    packs[PITCH].setValueWithUndo(step, pitchOffset);
}

recordStep(0, 100, 0.0);
recordStep(4, 80, 0.2);

Console.print(packs[VELOCITY].getValue(0)); // 0.787...
Console.print(packs[VELOCITY].getValue(4)); // 0.629...
```
```json:testMetadata:record-midi-to-steps
{
  "testable": false,
  "reason": "WithUndo methods require an active undo manager which is not available in automated REPL testing"
}
```
