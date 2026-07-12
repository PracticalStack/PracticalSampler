## getAutomationDataObject

**Examples:**

```javascript:inspect-automation-entries
// Title: Inspect automation entries and find assignments for a specific parameter
// Context: A custom automation UI needs to check whether a particular parameter
// is already assigned to a CC and which CC number controls it.

const var mah = Engine.createMidiAutomationHandler();

// Read the full automation state as an array of JSON objects
var data = mah.getAutomationDataObject();

// Each entry has Controller, Channel, Processor, Attribute, Start, End, etc.
for (entry in data)
{
    Console.print("CC" + entry.Controller + " ch:" + entry.Channel
                  + " -> " + entry.Processor + "." + entry.Attribute
                  + " [" + entry.Start + " .. " + entry.End + "]");
}
```
```json:testMetadata:inspect-automation-entries
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Array.isArray(data)", "value": true},
    {"type": "REPL", "expression": "data.length", "value": 0}
  ]
}
```

```javascript:check-parameter-automated
// Title: Check whether a specific parameter has a CC assignment
// Context: Before assigning a CC to a parameter, check the current
// automation data to see if an assignment already exists.

const var mah = Engine.createMidiAutomationHandler();

inline function isParameterAutomated(attributeId)
{
    local data = mah.getAutomationDataObject();

    for (entry in data)
    {
        if (entry.Attribute == attributeId)
            return true;
    }

    return false;
}

var result = isParameterAutomated("FilterCutoff");
Console.print(result); // true if assigned, false in a fresh session
```
```json:testMetadata:check-parameter-automated
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "result", "value": 0},
    {"type": "REPL", "expression": "Array.isArray(mah.getAutomationDataObject())", "value": true}
  ]
}
```

```javascript:remove-cc-assignment
// Title: Remove a specific CC assignment by controller number
// Context: A UI button removes the automation entry for a particular CC.
// Read the array, filter out the matching entry, write back.

const var mah = Engine.createMidiAutomationHandler();

inline function removeAutomationForCC(ccNumber)
{
    local data = mah.getAutomationDataObject();

    for (i = 0; i < data.length; i++)
    {
        if (data[i].Controller == ccNumber)
        {
            data.removeElement(i);
            break;
        }
    }

    mah.setAutomationDataFromObject(data);
}

removeAutomationForCC(20);

// Verify the operation completed (data is re-read after write-back)
var dataAfter = mah.getAutomationDataObject();
```
```json:testMetadata:remove-cc-assignment
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Array.isArray(dataAfter)", "value": true},
    {"type": "REPL", "expression": "dataAfter.length", "value": 0}
  ]
}
```
