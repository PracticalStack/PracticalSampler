## setMacroDataFromObject

**Examples:**

```javascript:clear-all-connections
// Title: Clear all macro connections on init
// Context: When using a custom preset model, clear macro state at
// startup so the preset loader starts from a known empty state

const var mh = Engine.createMacroHandler();
mh.setExclusiveMode(true);

// Clear all macro connections - the update callback fires once after this
mh.setMacroDataFromObject([]);

var data = mh.getMacroDataObject();
```
```json:testMetadata:clear-all-connections
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "data.length", "value": 0}
}
```

```javascript:custom-automation-macro-connections
// Title: Building macro connections with custom automation targets
// Context: Connect macro slots to custom automation parameters defined
// through UserPresetHandler.setCustomAutomation()

const var uph = Engine.createUserPresetHandler();
const var mh = Engine.createMacroHandler();
mh.setExclusiveMode(true);

// Custom automation requires the custom user preset model
uph.setUseCustomUserPresetModel(function(obj){}, function(){ return {}; }, false);

// Custom automation must be set up before macro connections reference it
uph.setCustomAutomation([
    {"ID": "ParamX", "min": 0.0, "max": 1.0, "defaultValue": 0.5, "connections": []},
    {"ID": "ParamY", "min": 0.0, "max": 1.0, "defaultValue": 0.5, "connections": []}
]);

// Connect macro 0 -> ParamX, macro 1 -> ParamY
mh.setMacroDataFromObject([
    {
        "MacroIndex": 0,
        "Processor": "Interface",
        "Attribute": "ParamX",
        "CustomAutomation": true,
        "FullStart": 0.0,
        "FullEnd": 1.0,
        "Start": 0.0,
        "End": 1.0,
        "Inverted": false,
        "Interval": 0.0,
        "Skew": 1.0
    },
    {
        "MacroIndex": 1,
        "Processor": "Interface",
        "Attribute": "ParamY",
        "CustomAutomation": true,
        "FullStart": 0.0,
        "FullEnd": 1.0,
        "Start": 0.0,
        "End": 1.0,
        "Inverted": false,
        "Interval": 0.0,
        "Skew": 1.0
    }
]);

var result = mh.getMacroDataObject();
```
```json:testMetadata:custom-automation-macro-connections
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "result.length", "value": 2},
    {"type": "REPL", "expression": "result[0].MacroIndex", "value": 0},
    {"type": "REPL", "expression": "result[1].MacroIndex", "value": 1}
  ]
}
```

**Pitfalls:**
- This method is destructive: it clears ALL existing macro connections before applying the new array. Passing a partial list removes connections not in the array. Use the read-modify-write pattern (`getMacroDataObject()` -> modify -> `setMacroDataFromObject()`) to change individual connections without losing others.
