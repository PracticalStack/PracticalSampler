## trim

**Examples:**

```javascript:prepare-trimmed-stereo-buffers
// Title: Prepare trimmed stereo buffers for file writing
// Context: After tail detection, export paths trim both channels to the same final length.

const var TOTAL_SAMPLES = 4096;
const var ACTIVE_SAMPLES = 2816;

const var left = Buffer.create(TOTAL_SAMPLES);
const var right = Buffer.create(TOTAL_SAMPLES);

left[ACTIVE_SAMPLES - 1] = 0.2;
right[ACTIVE_SAMPLES - 1] = 0.2;

local samplesToTrimAtEnd = TOTAL_SAMPLES - ACTIVE_SAMPLES;

const var channelsToWrite = [
    left.trim(0, samplesToTrimAtEnd),
    right.trim(0, samplesToTrimAtEnd)
];

Console.print(channelsToWrite[0].length); // 2816
Console.print(channelsToWrite[1].length); // 2816
```
```json:testMetadata:prepare-trimmed-stereo-buffers
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "channelsToWrite[0].length", "value": 2816},
    {"type": "REPL", "expression": "channelsToWrite[1].length", "value": 2816},
    {"type": "REPL", "expression": "left.length", "value": 4096}
  ]
}
```

**Pitfalls:**
- `trim()` does not modify the source Buffer in place; you must use the returned Buffer when building the export channel array.
