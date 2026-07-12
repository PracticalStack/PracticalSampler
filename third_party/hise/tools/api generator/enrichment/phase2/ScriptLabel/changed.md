## changed

**Examples:**

```javascript:clear-button-triggers-changed
// Title: Trigger callbacks after programmatic reset
// Context: Clear a search field from a button and notify listeners

const var searchLabel = Content.addLabel("SearchField", 12, 10);
searchLabel.set("text", "");
searchLabel.set("alignment", "left");
searchLabel.set("updateEachKey", true);
searchLabel.set("saveInPreset", false);

const var clearButton = Content.addButton("ClearSearch", 220, 10);
clearButton.set("text", "Clear");

reg lastQuery = "";

inline function onSearchChange(component, value)
{
    lastQuery = component.getValue();
    Console.print("Query: " + component.getValue()); // e.g. Query: Kick
}

inline function onClear(component, value)
{
    if (value)
    {
        searchLabel.set("text", "");
        searchLabel.changed();
    }
}

searchLabel.setControlCallback(onSearchChange);
clearButton.setControlCallback(onClear);
```
```json:testMetadata:clear-button-triggers-changed
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "searchLabel.setValue(\"Kick\") || clearButton.setValue(1) || clearButton.changed()", "value": false},
    {"type": "REPL", "expression": "lastQuery", "value": ""}
  ]
}
```
