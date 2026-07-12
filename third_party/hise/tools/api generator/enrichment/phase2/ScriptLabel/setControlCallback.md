## setControlCallback

**Examples:**

```javascript:live-search-callback
// Title: Live search field with per-key updates
// Context: Use updateEachKey so the callback runs while typing

const var searchLabel = Content.addLabel("SearchField", 12, 10);
searchLabel.set("text", "");
searchLabel.set("alignment", "left");
searchLabel.set("updateEachKey", true);
searchLabel.set("saveInPreset", false);

const var itemNames = ["Bass", "Pad", "Lead", "Keys"];

reg lastResultCount = 0;
reg lastFirstMatch = "";

inline function onSearchChange(component, value)
{
    local query = component.getValue().toLowerCase();
    local result = [];

    for (name in itemNames)
        if (query.length == 0 || name.toLowerCase().contains(query))
            result.push(name);

    lastResultCount = result.length;
    lastFirstMatch = result.length > 0 ? result[0] : "";
    Console.print(trace(result)); // e.g. ["Bass", "Pad"]
}

searchLabel.setControlCallback(onSearchChange);
```
```json:testMetadata:live-search-callback
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "searchLabel.setValue(\"ba\") || searchLabel.changed()", "value": false},
    {"type": "REPL", "expression": "lastResultCount", "value": 1},
    {"type": "REPL", "expression": "lastFirstMatch", "value": "Bass"}
  ]
}
```
