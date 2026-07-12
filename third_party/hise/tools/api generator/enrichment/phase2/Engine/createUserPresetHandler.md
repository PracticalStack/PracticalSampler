## createUserPresetHandler

**Examples:**

```javascript:preset-handler-post-load-broadcaster
// Title: Preset handler with post-load callback driving a broadcaster
// Context: Plugins that need to react to preset changes (update UI state,
// reset effects, refresh displays) connect the preset handler's post-load
// callback to a broadcaster that fans out the notification.

const var presetBroadcaster = Engine.createBroadcaster({
    "id": "preset loader",
    "args": ["newPresetFile"]
});

const var presetHandler = Engine.createUserPresetHandler();

// The post-callback fires after a preset finishes loading.
// Passing a broadcaster makes it the notification target.
presetHandler.setPostCallback(presetBroadcaster);

// Listeners react to preset changes independently
presetBroadcaster.addListener("stateTracker", "update current preset", function(newPresetFile)
{
    Console.print("Loaded: " + newPresetFile);
});

presetBroadcaster.addListener("effectReset", "reset effect state", function(newPresetFile)
{
    // Reset parameters that should not carry over between presets
    Console.print("Resetting effects after preset load");
});
```
```json:testMetadata:preset-handler-post-load-broadcaster
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "typeof presetHandler", "value": "object"},
    {"type": "REPL", "expression": "typeof presetBroadcaster", "value": "object"}
  ]
}
```
