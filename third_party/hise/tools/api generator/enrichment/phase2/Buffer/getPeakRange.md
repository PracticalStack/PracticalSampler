## getPeakRange

**Examples:**

```javascript:skip-empty-lanes-before-serialization
// Title: Skip empty lanes before preset serialization
// Context: Step-lane data is stored sparsely, so only lanes with real activity are serialized.

const var NUM_STEPS = 16;

inline function hasLaneActivity(stepBuffer)
{
    local range = stepBuffer.getPeakRange(0, NUM_STEPS);
    return range[0] != 0.0 || range[1] != 0.0;
}

const var laneA = Buffer.create(NUM_STEPS);
const var laneB = Buffer.create(NUM_STEPS);

laneB[3] = 0.75;

const var payload = [];
payload.push(hasLaneActivity(laneA) ? laneA.toBase64() : "EMPTY");
payload.push(hasLaneActivity(laneB) ? laneB.toBase64() : "EMPTY");

Console.print(payload[0]); // EMPTY
Console.print(payload[1].substring(0, 6)); // Buffer
```
```json:testMetadata:skip-empty-lanes-before-serialization
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "payload[0]", "value": "EMPTY"},
    {"type": "REPL", "expression": "payload[1].substring(0, 6)", "value": "Buffer"},
    {"type": "REPL", "expression": "hasLaneActivity(laneA)", "value": false},
    {"type": "REPL", "expression": "hasLaneActivity(laneB)", "value": true}
  ]
}
```

**Pitfalls:**
- Checking only `range[1]` can miss lanes that contain only negative values.
