## randInt

**Examples:**

```javascript:random-preset-selection
// Title: Random preset selection with duplicate avoidance
// Context: Selecting a random preset from a list while avoiding
// the currently loaded preset requires a retry loop.

const var presets = ["Warm Pad", "Bright Lead", "Deep Bass", "Soft Keys"];
var lastPresetIndex = -1;

inline function loadRandomPreset()
{
    local index = Math.randInt(0, presets.length);

    // Retry until we get a different preset (up to 10 attempts)
    local attempts = 0;
    while (index == lastPresetIndex && attempts < 10)
    {
        index = Math.randInt(0, presets.length);
        attempts++;
    }

    lastPresetIndex = index;
    return presets[index];
}

var result = loadRandomPreset();
Console.print(result);
```
```json:testMetadata:random-preset-selection
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lastPresetIndex >= 0 && lastPresetIndex < 4", "value": true},
    {"type": "REPL", "expression": "presets.indexOf(result) >= 0", "value": true}
  ]
}
```

```javascript:random-modulation-slot
// Title: Selecting random items from a modulation matrix
// Context: A "randomize" button picks random source and destination
// indices for modulation routing slots.

const var NUM_SOURCES = 8;
const var NUM_DESTINATIONS = 16;

inline function randomizeSlot()
{
    // Upper bound is exclusive: randInt(0, 8) returns 0-7
    local source = Math.randInt(0, NUM_SOURCES);
    local dest = Math.randInt(0, NUM_DESTINATIONS);
    return [source, dest];
}

var slot = randomizeSlot();
Console.print(slot[0] + ", " + slot[1]);
```
```json:testMetadata:random-modulation-slot
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "slot.length", "value": 2},
    {"type": "REPL", "expression": "slot[0] >= 0 && slot[0] < 8", "value": true},
    {"type": "REPL", "expression": "slot[1] >= 0 && slot[1] < 16", "value": true}
  ]
}
```
