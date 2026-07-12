## toBase64

**Examples:**

```javascript:serialize-for-clipboard
// Title: Serializing step sequencer data for clipboard and preset storage
// Context: A sequencer exports each channel's step data as a Base64 string
// for copy/paste and custom preset serialization.

const var spd = Engine.createAndRegisterSliderPackData(0);
spd.setNumSliders(8);
spd.setAllValues(0.0);
spd.setValue(0, 0.9);
spd.setValue(3, 0.5);

// Export to JSON-friendly format
inline function exportChannelData(data)
{
    local obj = {};

    if (hasNonZeroValues(data))
        obj.steps = data.toBase64();
    else
        obj.steps = "EMPTY";

    return obj;
}

// Check if any step has a non-zero value
inline function hasNonZeroValues(data)
{
    local buf = data.getDataAsBuffer();

    for (s in buf)
    {
        if (s != 0.0)
            return true;
    }

    return false;
}

var exported = exportChannelData(spd);
Console.print(exported.steps != "EMPTY"); // 1

// --- test-only ---
const var roundtrip = Engine.createAndRegisterSliderPackData(1);
roundtrip.fromBase64(exported.steps);
// --- end test-only ---
```
```json:testMetadata:serialize-for-clipboard
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "roundtrip.getValue(0)", "value": 0.9},
    {"type": "REPL", "expression": "roundtrip.getValue(3)", "value": 0.5},
    {"type": "REPL", "expression": "roundtrip.getNumSliders()", "value": 8}
  ]
}
```

The exported Base64 string can be stored in a JSON preset file or clipboard object, then restored with `fromBase64()`.
