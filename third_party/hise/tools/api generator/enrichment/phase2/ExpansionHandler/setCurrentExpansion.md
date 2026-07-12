## setCurrentExpansion

**Examples:**

```javascript:combobox-expansion-switching
// Title: ComboBox-driven expansion switching
// Context: Wire a ComboBox to switch the active expansion. The first item
// ("No Expansion") maps to an empty string which clears the selection.

const var eh = Engine.createExpansionHandler();

const var expansionNames = ["No Expansion"];

for (e in eh.getExpansionList())
    expansionNames.push(e.getProperties().Name);

const var ExpansionSelector = Content.getComponent("ExpansionSelector");
ExpansionSelector.set("items", expansionNames.join("\n"));

inline function onExpansionSelectorControl(component, value)
{
    local name = component.getItemText();

    // First item clears the expansion - pass empty string
    if (name == expansionNames[0])
        name = "";

    eh.setCurrentExpansion(name);
};

ExpansionSelector.setControlCallback(onExpansionSelectorControl);
```
```json:testMetadata:combobox-expansion-switching
{
  "testable": false,
  "skipReason": "Requires installed expansion packs and a pre-existing ComboBox UI component"
}
```

```javascript:close-button-deactivation
// Title: Close button that deactivates the current expansion
// Context: When a plugin loads as an expansion inside a shell product,
// provide a close button to return to the base product state.

const var eh = Engine.createExpansionHandler();

const var closeButton = Content.getComponent("CloseButton");
closeButton.set("allowCallbacks", "Clicks & Hover");

closeButton.setMouseCallback(function(event)
{
    this.data.hover = event.hover;
    this.repaint();

    if (event.clicked)
        eh.setCurrentExpansion("");
});
```
```json:testMetadata:close-button-deactivation
{
  "testable": false,
  "skipReason": "Requires installed expansion packs and UI interaction (mouse click on CloseButton)"
}
```
