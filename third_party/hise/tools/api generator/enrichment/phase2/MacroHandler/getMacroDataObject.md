## getMacroDataObject

**Examples:**

```javascript:query-used-macro-slots
// Title: Querying which macro slots are in use
// Context: Scan macro connections to determine which slots are occupied,
// e.g. to build a context menu with available/occupied indicators

const var mh = Engine.createMacroHandler();

var data = mh.getMacroDataObject();

// Build a set of occupied macro slot indices
const var usedSlots = [];

for (item in data)
{
    usedSlots.push(item.MacroIndex);
    Console.print("Macro " + item.MacroIndex + " -> "
                  + item.Attribute
                  + (item.CustomAutomation ? " (custom)" : ""));
}

Console.print("Used slots: " + usedSlots.length);
```
```json:testMetadata:query-used-macro-slots
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Array.isArray(data)", "value": true},
    {"type": "REPL", "expression": "usedSlots.length", "value": 0}
  ]
}
```

```javascript:read-modify-write-toggle
// Title: Read-modify-write to toggle a single macro connection
// Context: A context menu action that connects or disconnects a
// parameter to/from a specific macro slot without affecting others

// --- setup ---
const var uph = Engine.createUserPresetHandler();
uph.setUseCustomUserPresetModel(function(obj){}, function(){ return {}; }, false);
uph.setCustomAutomation([
    {"ID": "MyParam", "min": 0.0, "max": 1.0, "defaultValue": 0.5, "connections": []}
]);
// --- end setup ---

const var mh = Engine.createMacroHandler();
mh.setExclusiveMode(true);

const var PARAM_ID = "MyParam";
const var MACRO_SLOT = 0;

// Read current connections
var connections = mh.getMacroDataObject();

// Remove existing connection on this slot (if any)
for (item in connections)
{
    if (item.MacroIndex == MACRO_SLOT)
    {
        connections.remove(item);
        break;
    }
}

// Add new connection with custom automation target
connections.push({
    "MacroIndex": MACRO_SLOT,
    "Processor": "Interface",
    "Attribute": PARAM_ID,
    "CustomAutomation": true,
    "FullStart": 0.0,
    "FullEnd": 1.0,
    "Start": 0.0,
    "End": 1.0,
    "Inverted": false,
    "Interval": 0.0,
    "Skew": 1.0
});

// Write back the modified array
mh.setMacroDataFromObject(connections);

var result = mh.getMacroDataObject();
```
```json:testMetadata:read-modify-write-toggle
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "result.length", "value": 1},
    {"type": "REPL", "expression": "result[0].MacroIndex", "value": 0},
    {"type": "REPL", "expression": "result[0].Attribute", "value": "MyParam"}
  ]
}
```

**Pitfalls:**
- The returned array is not a reference to the internal state. After calling `remove()` or `push()` on the snapshot, you must call `setMacroDataFromObject()` to apply the changes - modifications to the array alone have no effect.
