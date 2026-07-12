## toBase64

**Examples:**

```javascript:serialize-lanes-with-empty-sentinel
// Title: Serialize per-lane step data with empty-lane sentinel
// Context: Preset models often store many lanes; writing "EMPTY" for silent lanes reduces payload size.

const var NUM_LANES = 4;
const var NUM_STEPS = 16;

inline function encodeLane(stepBuffer)
{
    local range = stepBuffer.getPeakRange(0, NUM_STEPS);
    local isEmpty = range[0] == 0.0 && range[1] == 0.0;
    return isEmpty ? "EMPTY" : stepBuffer.toBase64();
}

const var laneBuffers = [];

for (i = 0; i < NUM_LANES; i++)
    laneBuffers.push(Buffer.create(NUM_STEPS));

laneBuffers[1][2] = 1.0;
laneBuffers[3][8] = 0.4;

const var state = [];

for (b in laneBuffers)
    state.push(encodeLane(b));

Console.print(trace(state)); // ["EMPTY", "Buffer...", "EMPTY", "Buffer..."]
```
```json:testMetadata:serialize-lanes-with-empty-sentinel
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "state[0]", "value": "EMPTY"},
    {"type": "REPL", "expression": "state[1].substring(0, 6)", "value": "Buffer"},
    {"type": "REPL", "expression": "state[2]", "value": "EMPTY"},
    {"type": "REPL", "expression": "state[3].substring(0, 6)", "value": "Buffer"}
  ]
}
```

**Pitfalls:**
- Serializing every lane unconditionally creates large preset objects and removes the ability to quickly detect empty lanes.
