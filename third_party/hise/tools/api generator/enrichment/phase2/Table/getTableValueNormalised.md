## getTableValueNormalised

**Examples:**

```javascript:velocity-remapping-curve
// Title: Velocity remapping through a user-editable curve
// Context: A ScriptTable UI component lets the user shape the velocity response.
// The Table data handle queries the curve in the MIDI callback to remap velocity.

// Create a ScriptTable on the interface and get the data handle
const var VeloTable = Content.addTable("VeloTable", 150, 0);
VeloTable.set("width", 400);
VeloTable.set("height", 200);

const var table = VeloTable.registerAtParent(0);

// In the MIDI callback, remap velocity through the curve
inline function handleNoteOn()
{
    // Normalize velocity to 0.0-1.0, query the curve, scale back to 0-127
    local v = 127.0 * table.getTableValueNormalised(Message.getVelocity() / 127.0);
    Message.setVelocity(v);
}

function onNoteOn()
{
    handleNoteOn();
}
```
```json:testMetadata:velocity-remapping-curve
{
  "testable": false,
  "skipReason": "Callback uses Message.getVelocity() and Message.setVelocity() which require a MIDI callback context"
}
```

**Pitfalls:**
- Always normalize input to 0.0-1.0 before calling. Raw MIDI values (0-127) exceed the table range - any input above 1.0 returns the last table value, effectively flattening the response.
