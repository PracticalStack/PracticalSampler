## registerAtParent

**Examples:**

```javascript:register-table-for-runtime-lookup
// Title: Register UI curve once, then reuse it in note processing
// Context: The table editor stays in the UI layer, while note callbacks read a shared data handle.

const var velocityTable = Content.addTable("VelocityTable", 10, 10);
velocityTable.set("width", 280);
velocityTable.set("height", 120);

const var velocityData = velocityTable.registerAtParent(0);

inline function mapVelocity(rawVelocity)
{
    local normalized = rawVelocity / 127.0;
    local shaped = velocityData.getTableValueNormalised(normalized) * 127.0;
    return Math.range(Math.round(shaped), 1, 127);
}

function onNoteOn()
{
    Message.setVelocity(mapVelocity(Message.getVelocity()));
}
```
```json:testMetadata:register-table-for-runtime-lookup
{
  "testable": false,
  "skipReason": "registerAtParent requires a parent processor that exposes dynamic external data slots"
}
```

**Pitfalls:**
- Call `registerAtParent()` during setup, not inside note/control callbacks. Re-registering repeatedly adds avoidable runtime overhead.

**Cross References:**
- `ScriptTableData.getTableValueNormalised`
