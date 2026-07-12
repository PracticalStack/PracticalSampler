## set

**Examples:**

```javascript:custom-popup-sections
// Title: Custom popup combo box with headers, separators, and submenus
// Context: When useCustomPopup is enabled, the items string supports special
// formatting syntax for organizing complex option lists into sections.

const var cb = Content.addComboBox("FxSelector", 0, 0);
cb.set("useCustomPopup", true);
cb.set("saveInPreset", false);

// Headers (**...**) and separators (___) do not consume selection indices.
// Submenu syntax (Category::Item) creates nested popup menus.
// The value index counts only selectable items.
cb.set("items", [
    "**Filters**",
    "Filters::LowPass",
    "Filters::HighPass",
    "Filters::BandPass",
    "___",
    "**Dynamics**",
    "Dynamics::Compressor",
    "Dynamics::Gate",
    "___",
    "**Spatial**",
    "Spatial::Chorus",
    "Spatial::Delay",
    "Spatial::Reverb"
].join("\n"));

// Value 1 = "LowPass" (first selectable item, not the header)
// Value 4 = "Compressor" (headers and separators are skipped)
cb.setValue(1);
Console.print(cb.getItemText()); // "LowPass" -- submenu prefix stripped
```
```json:testMetadata:custom-popup-sections
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["LowPass"]},
    {"type": "REPL", "expression": "cb.getValue()", "value": 1}
  ]
}
```

```javascript:factory-pattern-namespace
// Title: Programmatic factory for combo boxes in a namespace
// Context: Create and configure combo boxes inside a namespace function that returns
// the configured component. This pattern keeps creation, styling, and callback
// assignment together for reusable selector components.

namespace Selectors
{
    inline function onSelectorChanged(component, value)
    {
        Console.print(component.getId() + ": " + parseInt(value));
    }

    inline function makeSelector(name, items, x, y)
    {
        local cb = Content.addComboBox(name, x, y);
        cb.set("width", 120);
        cb.set("height", 28);
        cb.set("items", items.join("\n"));
        cb.set("text", "Select...");
        cb.set("saveInPreset", false);
        cb.setControlCallback(onSelectorChanged);
        return cb;
    }
}

const var cbWaveform = Selectors.makeSelector("Waveform", ["Sine", "Saw", "Square"], 0, 0);
const var cbFilter = Selectors.makeSelector("FilterType", ["LP", "HP", "BP"], 140, 0);
```
```json:testMetadata:factory-pattern-namespace
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "cbWaveform.get(\"items\")", "value": "Sine\nSaw\nSquare"},
    {"type": "REPL", "expression": "cbWaveform.get(\"text\")", "value": "Select..."},
    {"type": "REPL", "expression": "cbFilter.get(\"width\")", "value": 120}
  ]
}
```
