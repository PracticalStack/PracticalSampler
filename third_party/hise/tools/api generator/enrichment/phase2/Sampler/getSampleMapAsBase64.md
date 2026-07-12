## getSampleMapAsBase64

**Examples:**

```javascript:base64-preset-persistence
// Title: Persist custom sample map state in a user preset
// Context: Store the entire sample map (including user-modified properties) as a
// base64 string inside a panel's value, which is saved with the user preset.

const var sampler = Synth.getSampler("MainSampler");
const var storagePanel = Content.getComponent("StoragePanel");

storagePanel.set("saveInPreset", true);

// Call after any sample property change to keep the preset in sync
inline function storeSampleMapData()
{
    local presetData = {
        "isCustom": true,
        "value": sampler.getSampleMapAsBase64()
    };

    storagePanel.setValue(presetData);
}

// Restore from preset: check the stored data and load accordingly
inline function restoreFromPreset(data)
{
    if (!isDefined(data.value) || data.value.length == 0)
    {
        sampler.clearSampleMap();
        return;
    }

    if (data.isCustom)
        sampler.loadSampleMapFromBase64(data.value);
    else
        sampler.loadSampleMap(data.value);
}
```

```json:testMetadata:base64-preset-persistence
{
  "testable": false,
  "skipReason": "Requires sampler with loaded samples and preset system"
}
```

The base64 string is zstd-compressed and contains the complete sample map ValueTree, including all per-sample properties. This is the recommended way to persist user-modified sample maps across preset save/load cycles.

**Cross References:**
- `Sampler.loadSampleMapFromBase64`
- `Sampler.getCurrentSampleMapId`
