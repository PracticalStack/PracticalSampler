## changed

**Examples:**

```javascript:cascading-dependent-combos
// Title: Cascading dependent combo boxes
// Context: Three combo boxes form a hierarchy: category -> option -> variant.
// When the category changes, the dependent lists are rebuilt and changed() is called
// to propagate the selection through the callback chain.

const var categories = ["Ambience", "Chambers", "Halls"];

// Simulated data: each category has different options and variants
const var optionData = {
    "Ambience": ["Small", "Medium", "Large"],
    "Chambers": ["Bright", "Dark", "Deep"],
    "Halls":    ["Concert", "Cathedral", "Studio"]
};

const var cbCategory = Content.addComboBox("Category", 0, 0);
const var cbOption = Content.addComboBox("Option", 0, 40);

cbCategory.set("items", categories.join("\n"));
cbCategory.set("saveInPreset", false);
cbOption.set("saveInPreset", false);

var lastOptionText = "";

inline function onCategoryChanged(component, value)
{
    local category = categories[parseInt(value) - 1];
    local options = optionData[category];

    // Rebuild the dependent combo box
    cbOption.set("items", options.join("\n"));

    // Select the first option in the new list (clamp to valid 1-based range)
    cbOption.setValue(1);

    // Trigger the dependent callback so downstream logic updates
    cbOption.changed();
}

inline function onOptionChanged(component, value)
{
    lastOptionText = component.getItemText();
}

cbCategory.setControlCallback(onCategoryChanged);
cbOption.setControlCallback(onOptionChanged);
```
```json:testMetadata:cascading-dependent-combos
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "cbCategory.setValue(2) || cbCategory.changed()", "value": false},
    {"type": "REPL", "expression": "lastOptionText", "value": "Bright"},
    {"type": "REPL", "expression": "cbOption.get(\"items\")", "value": "Bright\nDark\nDeep"}
  ]
}
```

**Pitfalls:**
- When rebuilding a dependent combo box, always call `setValue()` before `changed()`. If the previous selection index exceeds the new item count, the callback would receive an out-of-range value. Reset to 1 (or clamp to the new `max`) first.
