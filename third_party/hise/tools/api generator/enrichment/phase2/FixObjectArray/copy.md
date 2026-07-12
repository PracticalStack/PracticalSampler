## copy

**Examples:**

```javascript:multi-property-extraction
// Title: Multi-property column extraction for a visualization pipeline
// Context: A fixed-size object pool tracks visual elements with typed
// properties. Each timer tick updates the elements, then extracts
// property columns into Buffers for downstream consumption.

const var f = Engine.createFixObjectFactory({
    "x": 0,
    "y": 0.0,
    "seed": 0.0,
    "gain": 1.0
});

const var NUM_ELEMENTS = 4;
const var pool = f.createArray(NUM_ELEMENTS);

// Pre-allocate one Buffer per property column -- sizes must match
const var xBuf = Buffer.create(NUM_ELEMENTS);
const var yBuf = Buffer.create(NUM_ELEMENTS);
const var gainBuf = Buffer.create(NUM_ELEMENTS);

// Populate the pool
pool[0].x = 60; pool[0].y = 0.5; pool[0].gain = 0.9;
pool[1].x = 64; pool[1].y = 1.2; pool[1].gain = 0.7;
pool[2].x = 67; pool[2].y = 0.8; pool[2].gain = 0.5;
pool[3].x = 72; pool[3].y = 1.5; pool[3].gain = 0.3;

// Extract each property into its own Buffer
pool.copy("x", xBuf);
pool.copy("y", yBuf);
pool.copy("gain", gainBuf);

Console.print(xBuf[0]);    // 60.0
Console.print(gainBuf[2]); // 0.5
```
```json:testMetadata:multi-property-extraction
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "xBuf[0]", "value": 60.0},
    {"type": "REPL", "expression": "xBuf[2]", "value": 67.0},
    {"type": "REPL", "expression": "gainBuf[2]", "value": 0.5},
    {"type": "REPL", "expression": "yBuf[1]", "value": 1.2}
  ]
}
```

```javascript:update-then-extract
// Title: Update-then-extract pattern in a timer callback
// Context: For-in gives live references -- modify properties in-place,
// then bulk-extract the updated values with copy().

const var f = Engine.createFixObjectFactory({
    "value": 0.0,
    "active": false
});

const var NUM_SLOTS = 4;
const var slots = f.createArray(NUM_SLOTS);
const var valueBuf = Buffer.create(NUM_SLOTS);

// Initialize with non-zero values so the decay is observable
slots[0].value = 1.0;
slots[1].value = 0.8;
slots[2].value = 0.6;
slots[3].value = 0.4;

// Simulate decay: scale each element's value, then extract
for (obj in slots)
    obj.value *= 0.5;

slots.copy("value", valueBuf);

// valueBuf now contains the decayed values as a flat float array
Console.print(valueBuf[0]); // 0.5 (1.0 * 0.5)
Console.print(valueBuf[3]); // 0.2 (0.4 * 0.5)
```
```json:testMetadata:update-then-extract
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "valueBuf[0]", "value": 0.5},
    {"type": "REPL", "expression": "valueBuf[1]", "value": 0.4},
    {"type": "REPL", "expression": "valueBuf[3]", "value": 0.2}
  ]
}
```

**Pitfalls:**
- When extracting multiple properties in sequence, allocate all target Buffers at init time with the same size as the array. Creating Buffers inside a timer callback defeats the allocation-free design of FixObjectArray.
