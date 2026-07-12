## setUseCustomUserPresetModel

**Examples:**

```javascript:custom-data-model-structured
// Title: Custom data model with structured save/load for a multi-channel instrument
// Context: When a plugin has hundreds of parameters organized in a structured
// hierarchy (e.g., per-channel settings), the default flat component-value
// serialization is insufficient. The custom data model lets you serialize
// arbitrary JSON and control exactly what goes into each preset.

const var NUM_CHANNELS = 4;
const var uph = Engine.createUserPresetHandler();

// Assume filters[] and compressors[] are arrays of module references
// obtained via Synth.getEffect() during init

inline function onPresetLoad(obj)
{
    if (!isDefined(obj))
        return;

    // Restore automation values from the structured preset data
    uph.updateAutomationValues(obj.AutomatedData, SyncNotification, false);

    // Restore additional UI component state (saveInPreset components)
    uph.updateSaveInPresetComponents(obj.AdditionalData);

    // Restore module bypass states manually
    for (i = 0; i < NUM_CHANNELS; i++)
    {
        filters[i].setBypassed(!obj.ChannelStates[i].Filter);
        compressors[i].setBypassed(!obj.ChannelStates[i].Comp);
    }

    // Sync UI components with the restored module states
    uph.updateConnectedComponentsFromModuleState();
};

inline function onPresetSave(name)
{
    local obj = {};

    // Capture all custom automation slot values
    obj.AutomatedData = uph.createObjectForAutomationValues();

    // Capture saveInPreset component values
    obj.AdditionalData = uph.createObjectForSaveInPresetComponents();

    // Manually capture state not covered by automation or components
    obj.ChannelStates = [];

    for (i = 0; i < NUM_CHANNELS; i++)
    {
        obj.ChannelStates.push({
            "Filter": !filters[i].isBypassed(),
            "Comp": !compressors[i].isBypassed()
        });
    }

    return obj;
};

// Enable the custom data model BEFORE calling setCustomAutomation
uph.setUseCustomUserPresetModel(onPresetLoad, onPresetSave, false);
```
```json:testMetadata:custom-data-model-structured
{
  "testable": false,
  "skipReason": "Requires a full module tree with filters[] and compressors[] module references plus a preset save/load cycle."
}
```

**Pitfalls:**
- The save callback's `presetName` argument is currently always "Unused" when called through the CustomStateManager. Do not rely on it for generating file paths or display names.
