## setVelocity

**Examples:**

```javascript:velocity-curve-reshaping
// Title: Velocity curve reshaping via lookup table
// Context: A velocity table modifies the incoming velocity to match
// the instrument's dynamic response. The table provides a normalized
// curve (0.0-1.0 input/output) that reshapes the velocity distribution.

const var veloTable = Content.addTable("VeloTable", 10, 10);
veloTable.set("height", 200);
veloTable.set("width", 400);

const var table = veloTable.registerAtParent(0);

function onNoteOn()
{
    // Map velocity through the table curve
    local v = 127.0 * table.getTableValueNormalised(Message.getVelocity() / 127.0);

    Message.setVelocity(parseInt(Math.max(1, v)));
}
```
```json:testMetadata:velocity-curve-reshaping
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

**Pitfalls:**
- `setVelocity()` only works in `onNoteOn` - it rejects note-off events. Use `setGain()` instead if you need to modify the level of both note-on and note-off events.
- Always ensure the velocity is at least 1. A velocity of 0 on a note-on is technically valid in HISE (it does not convert to note-off), but downstream MIDI output may interpret it as a note-off per the MIDI specification.
