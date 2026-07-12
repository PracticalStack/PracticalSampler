## fromBase64

**Examples:**

```javascript:restore-sparse-lanes-with-scratch-buffer
// Title: Restore sparse lane state with one reusable scratch buffer
// Context: Import routines decode many payloads, so they reuse one temp Buffer and handle "EMPTY" entries explicitly.

const var NUM_STEPS = 16;

const var source = Buffer.create(NUM_STEPS);
source[2] = 1.0;

const var encodedLanes = ["EMPTY", source.toBase64(), "EMPTY"];
const var decodedLanes = [];
const var scratch = Buffer.create(NUM_STEPS);

for (entry in encodedLanes)
{
    if (entry == "EMPTY")
    {
        decodedLanes.push(Buffer.create(NUM_STEPS));
        continue;
    }

    local ok = scratch.fromBase64(entry);

    if (!ok)
    {
        decodedLanes.push(Buffer.create(NUM_STEPS));
        continue;
    }

    local lane = Buffer.create(NUM_STEPS);
    scratch >> lane;
    decodedLanes.push(lane);
}
```
```json:testMetadata:restore-sparse-lanes-with-scratch-buffer
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "decodedLanes.length", "value": 3},
    {"type": "REPL", "expression": "decodedLanes[0].getMagnitude(0, NUM_STEPS)", "value": 0},
    {"type": "REPL", "expression": "decodedLanes[1][2]", "value": 1},
    {"type": "REPL", "expression": "decodedLanes[2].length", "value": 16}
  ]
}
```

**Pitfalls:**
- Ignoring the return value from `fromBase64()` can leak partially initialized state into the restored model.
