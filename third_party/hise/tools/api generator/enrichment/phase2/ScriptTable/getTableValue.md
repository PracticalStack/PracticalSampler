## getTableValue

**Examples:**

```javascript:velocity-curve-transfer-function
// Title: Use a ScriptTable as a velocity transfer function
// Context: A MIDI script shapes incoming velocity with a user-editable curve.

const var velocityCurve = Content.addTable("VelocityCurve", 10, 10);
velocityCurve.set("width", 240);
velocityCurve.set("height", 100);
velocityCurve.addTablePoint(0.5, 0.25);

inline function applyVelocityCurve(inputVelocity)
{
    local normalized = inputVelocity / 127.0;
    local mapped = velocityCurve.getTableValue(normalized);
    return Math.max(1, Math.round(mapped * 127.0));
}

function onNoteOn()
{
    Message.setVelocity(applyVelocityCurve(Message.getVelocity()));
}
```

```json:testMetadata:velocity-curve-transfer-function
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Math.round(velocityCurve.getTableValue(0.5) * 100) / 100", "value": 0.5},
    {"type": "REPL", "expression": "applyVelocityCurve(127)", "value": 127}
  ]
}
```

**Pitfalls:**
- `getTableValue()` expects a normalized input. If you pass MIDI-domain values directly, the table behaves like a near-constant mapping.

**Cross References:**
- `ScriptTable.registerAtParent`
- `ScriptTableData.getTableValueNormalised`
