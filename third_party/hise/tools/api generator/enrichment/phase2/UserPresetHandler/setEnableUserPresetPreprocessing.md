## setEnableUserPresetPreprocessing

**Examples:**

```javascript:complex-data-migration
// Title: Migrating presets from an older version with complex data unpacking
// Context: When a plugin update changes data formats (e.g., replacing an
// AudioWaveform with a Sampler-based sample importer), preprocessing with
// shouldUnpackComplexData=true allows the pre-callback to inspect and
// transform Base64-encoded data inside the preset.

const var uph = Engine.createUserPresetHandler();

// Enable preprocessing with complex data unpacking
// - true: pre-callback receives JSON instead of a File
// - true: Base64 data and JSON-encoded strings are decoded
uph.setEnableUserPresetPreprocessing(true, true);

uph.setPreCallback(function(data)
{
    // data.Content is an array of component value objects
    for (c in data.Content)
    {
        if (c.id == "OldWaveform")
        {
            // c.data is now the decoded Base64 content (not a raw string)
            // Transform it into the new format and inject a replacement entry
            var newEntry = {
                "type": "ScriptPanel",
                "id": "SampleDropper",
                "value": {
                    "isCustom": true,
                    "value": c.data
                }
            };

            data.Content.push(newEntry);
            break;
        }
    }
});
```
```json:testMetadata:complex-data-migration
{
  "testable": false,
  "skipReason": "Requires a preset file with Base64-encoded data from an older plugin version."
}
```

```javascript:version-migration-simple
// Title: Version-aware migration without complex data unpacking
// Context: The most common use case: inspect the preset version and
// add/rename/remap simple control values. Complex data unpacking is
// not needed when you only work with scalar values.

const var uph = Engine.createUserPresetHandler();

// Enable preprocessing but skip complex data unpacking (faster)
uph.setEnableUserPresetPreprocessing(true, false);

uph.setPreCallback(function(presetData)
{
    if (uph.isOldVersion(presetData.version))
    {
        // Add controls that were introduced in newer versions
        presetData.Content.push({
            "type": "ScriptButton",
            "id": "btnNewFeature",
            "value": true
        });

        // Remap a renamed control
        for (c in presetData.Content)
        {
            if (c.id == "knbOldGate")
            {
                c.id = "knbGateHold";
                c.value = Math.max(c.value, 10);
                break;
            }
        }
    }
});
```
```json:testMetadata:version-migration-simple
{
  "testable": false,
  "skipReason": "Requires a preset load with an older version preset file."
}
```
