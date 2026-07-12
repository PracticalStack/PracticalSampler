## setControllerNumbersInPopup

**Examples:**

```javascript:curated-cc-popup-exclusive
// Title: Configure a curated CC popup with readable names
// Context: An instrument exposes a fixed set of CC-controllable parameters.
// The popup should only show relevant CC numbers with descriptive labels
// instead of the default full list of 128 CCs.

const var mah = Engine.createMidiAutomationHandler();

// Define the CC numbers available for automation
const var NUM_SLOTS = 4;
const var CC_START = 20;
const var ccNumbers = [];
const var names = [];

for (i = 0; i < NUM_SLOTS; i++)
{
    ccNumbers.push(CC_START + i);
    names.push("Slot " + (i + 1));
}

// One CC per parameter - grays out already-assigned CCs in the popup
mah.setExclusiveMode(true);

// Only these CC numbers appear in the right-click automation popup
mah.setControllerNumbersInPopup(ccNumbers);

// Set the popup section header and per-CC display names.
// The nameArray is indexed by CC number, so populate the correct indices.
var nameArray = [];
for (i = 0; i < NUM_SLOTS; i++)
    nameArray[CC_START + i] = names[i];

mah.setControllerNumberNames("Automation", nameArray);
```
```json:testMetadata:curated-cc-popup-exclusive
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "ccNumbers.length", "value": 4},
    {"type": "REPL", "expression": "ccNumbers[3]", "value": 23},
    {"type": "REPL", "expression": "nameArray[21]", "value": "Slot 2"},
    {"type": "REPL", "expression": "!isDefined(nameArray[0])", "value": true}
  ]
}
```

When a CC filter is set via `setControllerNumbersInPopup()`, the popup layout changes from a nested submenu (with a "Learn" option at the top) to a flat list showing only the specified CC numbers. This is the preferred layout for instruments with a small, fixed set of controllable parameters.

**Cross References:**
- `MidiAutomationHandler.setControllerNumberNames`
- `MidiAutomationHandler.setExclusiveMode`
