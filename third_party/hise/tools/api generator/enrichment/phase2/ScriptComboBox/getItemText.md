## getItemText

**Examples:**

```javascript:load-sample-map-by-text
// Title: Load a sample map by selected item text
// Context: The combo box items match sample map names exactly, so getItemText()
// provides the string needed by loadSampleMap() without any index conversion.

const var mapList = Sampler.getSampleMapList();

const var cbMap = Content.addComboBox("MapSelector", 0, 0);
cbMap.set("items", mapList.join("\n"));
cbMap.set("saveInPreset", false);

inline function onMapChanged(component, value)
{
    Console.print("Selected map: " + component.getItemText());
}

cbMap.setControlCallback(onMapChanged);
```
```json:testMetadata:load-sample-map-by-text
{
  "testable": false,
  "skipReason": "Requires a sampler module and loaded sample maps to produce meaningful output"
}
```

```javascript:guard-no-options
// Title: Handle empty and out-of-range return values
// Context: getItemText() returns an empty string when the value is 0 (nothing
// selected) and "No options" when the value exceeds the item count.
// Guard against both in cascading selector patterns where items may be rebuilt.

const var cbGuard = Content.addComboBox("GuardCombo", 0, 0);
cbGuard.set("items", "Ambience\nChambers\nHalls");
cbGuard.set("saveInPreset", false);

// Explicitly reset to 0 (nothing selected) -- getItemText() returns ""
cbGuard.setValue(0);
Console.print(cbGuard.getItemText()); // ""

cbGuard.setValue(2);
Console.print(cbGuard.getItemText()); // "Chambers"
```
```json:testMetadata:guard-no-options
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["", "Chambers"]}
}
```

**Pitfalls:**
- When `useCustomPopup` is enabled, submenu prefixes are stripped from the returned text. An item stored as `"Filters::LowPass"` returns `"LowPass"`, not the full path. If you need the full path for lookup purposes, maintain a parallel data array indexed by `parseInt(value) - 1` instead of relying on `getItemText()`.
