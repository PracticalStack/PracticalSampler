## setControllerNumberNames

**Examples:**

```javascript:named-cc-popup-slots
// Title: Build a named CC popup for a fixed set of automation slots
// Context: An instrument reserves a contiguous block of CCs for automation.
// The popup should show "Slot 1" through "Slot 4" instead of "CC#20" etc.

const var mah = Engine.createMidiAutomationHandler();

const var NUM_SLOTS = 4;
const var CC_START = 20;
const var ccNumbers = [];

// Build the CC filter and the name array in one pass.
// The name array is indexed by CC number - only populated indices
// get custom names; all others fall back to "CC#N".
var names = [];

for (i = 0; i < NUM_SLOTS; i++)
{
    ccNumbers.push(CC_START + i);
    names[CC_START + i] = "Slot " + (i + 1);
}

mah.setControllerNumbersInPopup(ccNumbers);

// "Automation" replaces the default "MIDI CC" section header in the popup
mah.setControllerNumberNames("Automation", names);
```
```json:testMetadata:named-cc-popup-slots
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "ccNumbers.length", "value": 4},
    {"type": "REPL", "expression": "isDefined(names[20]) && !isDefined(names[19])", "value": true},
    {"type": "REPL", "expression": "names[22]", "value": "Slot 3"}
  ]
}
```

The `nameArray` parameter uses sparse indexing: the name at array index N maps to CC number N. For a small set of higher-numbered CCs, you only need to set the entries at those indices. Unset indices fall back to the default "CC#N" format.
