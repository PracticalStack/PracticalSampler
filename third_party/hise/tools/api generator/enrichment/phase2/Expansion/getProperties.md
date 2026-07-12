## getProperties

**Examples:**

```javascript:expansion-name-selector
// Title: Populating a combo box with expansion names
// Context: Build an expansion selector by reading each expansion's
// Name property and listing them in a combo box.

const var expHandler = Engine.createExpansionHandler();
const var ExpansionSelector = Content.getComponent("ExpansionSelector");

const var names = ["No Expansion"];

for (e in expHandler.getExpansionList())
    names.push(e.getProperties().Name);

ExpansionSelector.set("items", names.join("\n"));

inline function onExpansionSelectorControl(component, value)
{
    local name = component.getItemText();

    // First item resets to no expansion
    if (name == "No Expansion")
        name = "";

    expHandler.setCurrentExpansion(name);
};

ExpansionSelector.setControlCallback(onExpansionSelectorControl);
```

```json:testMetadata:expansion-name-selector
{
  "testable": false,
  "skipReason": "Requires installed expansion packs"
}
```
