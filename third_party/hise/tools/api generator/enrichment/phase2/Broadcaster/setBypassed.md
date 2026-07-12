## setBypassed

**Examples:**

```javascript:batch-update-with-bypass
// Title: Batch UI update with bypass - suppress intermediate broadcasts
// Context: When loading a preset that changes many values simultaneously,
// bypass the broadcaster to prevent listeners from reacting to each
// intermediate state. Unbypass with sendMessageIfEnabled to send the
// final state once at the end.

const var channelBc = Engine.createBroadcaster({
    "id": "ChannelState",
    "args": ["index"]
});

var channelLog = [];

channelBc.addListener("", "updateUI", function(index)
{
    channelLog.push(index);
});

// During preset load, suppress all intermediate updates
channelBc.setBypassed(true, false, false);

// ... restore many values from preset data ...
channelBc.sendSyncMessage([5]);  // Stored but not dispatched
channelBc.sendSyncMessage([3]);  // Stored but not dispatched

// Unbypass and resend the final state
// Note: true = synchronous dispatch (despite the parameter being named "async")
channelBc.setBypassed(false, true, true);
// Listener fires once with value 3
```
```json:testMetadata:batch-update-with-bypass
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "channelLog.length", "value": 1},
    {"type": "REPL", "expression": "channelLog[0]", "value": 3}
  ]
}
```

```javascript:one-shot-self-bypass
// Title: One-shot broadcaster that permanently disables itself
// Context: A broadcaster that fires once on first interaction then permanently
// disables itself. Useful for lazy initialization patterns.

const var initLoader = Engine.createBroadcaster({
    "id": "InitLoader",
    "args": ["component", "event"]
});

// --- setup ---
const var RootPanel = Content.addPanel("RootPanel", 0, 0);
RootPanel.set("width", 200);
RootPanel.set("height", 200);
RootPanel.set("saveInPreset", false);
// --- end setup ---

initLoader.attachToComponentMouseEvents(RootPanel, "Clicks & Hover", "hover");

var loadCount = 0;

initLoader.addListener("", "loadOnce", function(component, event)
{
    // Permanently disable - this broadcaster never fires again
    initLoader.setBypassed(true, false, false);
    loadCount++;
});
```
```json:testMetadata:one-shot-self-bypass
{
  "testable": false,
  "skipReason": "Mouse events require physical user interaction that cannot be triggered programmatically from script"
}
```

**Pitfalls:**
- The third parameter (`async`) has inverted semantics relative to its name. Passing `true` causes **synchronous** dispatch, and `false` causes **asynchronous** dispatch. Use `SyncNotification`/`AsyncNotification` constants for clarity: `setBypassed(false, true, SyncNotification)`.
