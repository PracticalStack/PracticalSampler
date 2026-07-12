## restoreScriptControls

**Examples:**

```javascript:preset-load-iterate-processors
// Title: Restoring script processor controls from a preset object
// Context: When loading a factory preset stored as a JSON object, restore
// each script processor's UI controls from the saved base64 strings.
// This avoids recompilation -- only knob/slider/button values change.

const var presetStoragePanel = Content.getComponent("PresetStorage");

inline function onPresetLoad(component, value)
{
    if (typeof value != "object")
        return;

    local midiIds = Synth.getIdList("MidiProcessor");

    for (id in midiIds)
    {
        // Skip if this processor wasn't in the saved preset
        if (typeof value.midiProcessors[id] == void)
            continue;

        local mp = Synth.getMidiProcessor(id);
        mp.restoreScriptControls(value.midiProcessors[id]);
    }
}

presetStoragePanel.setControlCallback(onPresetLoad);
```
```json:testMetadata:preset-load-iterate-processors
{
  "testable": false,
  "skipReason": "Callback-driven pattern requiring UI component and existing script processors in the module tree"
}
```
