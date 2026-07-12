## getCurrentSampleMapId

**Examples:**

```javascript:current-map-id
// Title: Detect whether the loaded map is a custom import or a factory map
// Context: Maps loaded via loadSampleMapFromJSON() get the ID "CustomJSON".
// Use this to distinguish custom user imports from factory sample maps.

const var sampler = Synth.getSampler("MainSampler");

inline function isCustomMap()
{
    return sampler.getCurrentSampleMapId() == "CustomJSON";
}

// Sync a ComboBox selector with the currently loaded factory map
inline function syncSelectorWithCurrentMap()
{
    local currentId = sampler.getCurrentSampleMapId();

    if (currentId.length == 0 || currentId == "CustomJSON")
    {
        // No factory map loaded - clear the selector
        mapSelector.setValue(0);
        return;
    }

    // Find the map in the list and select it
    local mapList = Sampler.getSampleMapList();
    local index = mapList.indexOf(currentId);

    if (index != -1)
        mapSelector.setValue(index + 1);
}
```

```json:testMetadata:current-map-id
{
  "testable": false,
  "skipReason": "Requires sampler with loaded sample map"
}
```

Returns an empty string if no sample map is loaded. Returns `"CustomJSON"` for maps loaded via `loadSampleMapFromJSON()`. For factory maps loaded via `loadSampleMap()`, returns the pool reference ID (e.g., `"Strings/Sustain"`).
