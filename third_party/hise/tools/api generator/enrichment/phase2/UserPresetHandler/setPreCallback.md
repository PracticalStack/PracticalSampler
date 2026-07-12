## setPreCallback

**Examples:**

```javascript:preset-migration-preprocessing
// Title: Preset migration with preprocessing - adding new controls and remapping values
// Context: When a plugin update adds new UI controls or renames existing ones,
// older presets lack those entries. With preprocessing enabled, the pre-callback
// receives a JSON object that can be modified before loading.

const var uph = Engine.createUserPresetHandler();
uph.setEnableUserPresetPreprocessing(true, false);

uph.setPreCallback(function(presetData)
{
    // Check the preset's version against the current plugin version
    if (uph.isOldVersion(presetData.version))
    {
        // Rename a control that was changed in an update
        for (c in presetData.Content)
        {
            if (c.id == "knbOldName")
            {
                c.id = "knbNewName";
                break;
            }
        }

        // Add a control that didn't exist in older presets
        presetData.Content.push({
            "type": "ScriptButton",
            "id": "btnNewFeature",
            "value": true
        });
    }
});
```
```json:testMetadata:preset-migration-preprocessing
{
  "testable": false,
  "skipReason": "Requires a preset load with an actual preset file containing older version data."
}
```

```javascript:broadcaster-as-precallback
// Title: Using a Broadcaster as the pre-callback for decoupled preset reactions
// Context: When multiple systems need to react before a preset loads (e.g.,
// saving FX lock states, stopping transport, preparing UI), pass a Broadcaster
// to setPreCallback instead of a single function. Each system adds its own
// listener without coupling to the preset handler.

const var uph = Engine.createUserPresetHandler();

const var preLoadBroadcaster = Engine.createBroadcaster({
    "id": "presetPreLoad",
    "args": ["presetFile"],
    "tags": ["preset"]
});

// Enable queued processing so listeners execute in registration order
preLoadBroadcaster.setEnableQueue(true);

// Pass the broadcaster directly as the pre-callback
uph.setPreCallback(preLoadBroadcaster);

// Object to store locked parameter values across preset loads
var lockedValues = {};

// System 1: Save locked parameter values before the preset overwrites them
preLoadBroadcaster.addListener(lockedValues, "save locked state",
    function(presetFile)
{
    this.filterFreq = filterKnob.getValue();
    this.mixLevel = mixKnob.getValue();
});

// System 2: Stop transport before preset load
preLoadBroadcaster.addListener("", "stop transport",
    function(presetFile)
{
    transportHandler.stopInternalClock(0);
});
```
```json:testMetadata:broadcaster-as-precallback
{
  "testable": false,
  "skipReason": "Requires a preset load to trigger the broadcaster chain."
}
```
