## fromBase64

**Examples:**

```javascript:restore-from-preset-format
// Title: Restoring step data from a custom preset format
// Context: A sequencer stores per-channel step data as Base64 strings
// in a JSON preset. On restore, each channel's data is loaded from
// the string, with "EMPTY" as a sentinel for cleared channels.

const var spd = Engine.createAndRegisterSliderPackData(0);
spd.setNumSliders(8);
spd.setUsePreallocatedLength(8);

// Simulate preset data with Base64-encoded step values
spd.setAllValues(0.0);
spd.setValue(0, 0.9);
spd.setValue(3, 0.5);
var savedState = spd.toBase64();

// Clear and restore
spd.setAllValues(0.0);

inline function restoreChannel(data, b64)
{
    if (b64 == "EMPTY")
        data.setAllValuesWithUndo(0.0);
    else
        data.fromBase64(b64);
}

restoreChannel(spd, savedState);
Console.print(spd.getValue(0)); // 0.9
Console.print(spd.getValue(3)); // 0.5
```
```json:testMetadata:restore-from-preset-format
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "spd.getValue(0)", "value": 0.9},
    {"type": "REPL", "expression": "spd.getValue(3)", "value": 0.5}
  ]
}
```

Note that `fromBase64()` replaces the entire buffer and adjusts the slider count to match the decoded data size. If using preallocated lengths, the buffer size may change to match the encoded data.
