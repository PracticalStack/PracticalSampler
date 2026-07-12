## createModifiers

**Examples:**

```javascript:shared-modifier-schema
// Title: Create one shared modifier schema for many sliders
// Context: A larger interface uses many ScriptSlider controls and keeps gesture mappings consistent.

const var sliderIds = ["GainA", "GainB", "GainC", "GainD"];
const var sliders = [];

for (id in sliderIds)
    sliders.push(Content.addKnob(id, 0, 0));

// Create constants once from one slider and reuse them.
const var mods = sliders[0].createModifiers();

for (s in sliders)
{
    s.setModifiers(mods.TextInput, mods.shiftDown);
    s.setModifiers(mods.FineTune, mods.ctrlDown | mods.cmdDown);
    s.setModifiers(mods.ResetToDefault, [mods.doubleClick, mods.noKeyModifier]);
}
```
```json:testMetadata:shared-modifier-schema
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "mods.TextInput", "value": "TextInput"},
    {"type": "REPL", "expression": "mods.ResetToDefault", "value": "ResetToDefault"},
    {"type": "REPL", "expression": "mods.doubleClick > 0", "value": true}
  ]
}
```

**Cross References:**
- `ScriptSlider.setModifiers`
