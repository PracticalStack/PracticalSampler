## assertEqual

**Examples:**

```javascript:verifying-data-array-dimensions
// Title: Verifying data array dimensions
// Context: When a multi-dimensional data structure must have consistent
// sizes, assertEqual catches mismatches before they cause index errors.

const var NUM_CHANNELS = 4;
const var NUM_MODES = 3;
const var NUM_BANKS = 2;

const var dataPackList = [];
for (i = 0; i < NUM_BANKS * NUM_CHANNELS * NUM_MODES; i++)
    dataPackList.push(i);

Console.assertEqual(NUM_BANKS * NUM_CHANNELS * NUM_MODES, dataPackList.length);
```
```json:testMetadata:verifying-data-array-dimensions
{
  "testable": true,
  "verifyScript": {
    "type": "REPL",
    "expression": "dataPackList.length",
    "value": 24
  }
}
```


```javascript:unit-testing-a-string-transformation
// Title: Unit-testing a string transformation function
// Context: assertEqual works well for inline unit tests that run
// during init and are stripped in exported builds.

inline function transformName(name, oldMode, newMode)
{
    local modeNames = ["Low", "Mid", "High", "Full"];
    local oldName = modeNames[oldMode];
    local newName = modeNames[newMode];
    
    // Find and replace the mode name (handle numbered suffixes)
    if (name.indexOf(oldName) >= 0)
        return name.replace(oldName, newName);
    
    return name;
}

Console.assertEqual(transformName("Drive Low1", 0, 3), "Drive Full1");
Console.assertEqual(transformName("Drive Full9", 3, 0), "Drive Low9");
Console.assertEqual(transformName("Mixer 2 PanMid", 1, 3), "Mixer 2 PanFull");
```
```json:testMetadata:unit-testing-a-string-transformation
{
  "testable": true,
  "verifyScript": {
    "type": "REPL",
    "expression": "typeof(transformName)",
    "value": "object"
  }
}
```

