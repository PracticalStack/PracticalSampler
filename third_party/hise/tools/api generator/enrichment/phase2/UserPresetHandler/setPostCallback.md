## setPostCallback

**Examples:**

```javascript:broadcaster-post-callback
// Title: Using a Broadcaster as the post-callback to drive multiple UI updates
// Context: After a preset loads, many UI systems need to update: the preset
// name display, panel repaints, page visibility, transport state. Instead of
// putting all this logic in one callback function, pass a Broadcaster as the
// post-callback and let each system register its own listener.

const var uph = Engine.createUserPresetHandler();

const var postLoadBroadcaster = Engine.createBroadcaster({
    "id": "presetPostLoad",
    "args": ["presetFile"],
    "tags": ["preset", "postloading"]
});

// Pass the broadcaster directly - it acts as the post-callback
uph.setPostCallback(postLoadBroadcaster);

// Assume presetNameLabel and mixerPanel are defined ScriptPanel references
// Listener 1: Update preset name display
postLoadBroadcaster.addListener(presetNameLabel, "update preset name",
    function(presetFile)
{
    local name = Engine.getCurrentUserPresetName();
    this.set("text", name.length > 0 ? name : "Init");
});

// Listener 2: Refresh custom panels
postLoadBroadcaster.addListener(mixerPanel, "refresh mixer",
    function(presetFile)
{
    this.repaint();
});
```
```json:testMetadata:broadcaster-post-callback
{
  "testable": false,
  "skipReason": "Requires a preset load to trigger the broadcaster chain."
}
```
