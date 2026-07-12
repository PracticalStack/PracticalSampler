## callWithDelay

**Examples:**

```javascript:deferred-action-after-event
// Title: Deferred action after preset load completes
// Context: After loading a preset, some actions need to happen after a short
// delay to allow the UI to settle. callWithDelay schedules a one-shot
// function call independently of the broadcaster's listener system.

const var presetBc = Engine.createBroadcaster({
    "id": "PresetLifecycle",
    "args": ["presetName"]
});

var delayedResult = "";

// After preset load completes, schedule a deferred transport start
presetBc.addListener("", "onLoad", function(presetName)
{
    // Start playback 60ms after load, giving the UI time to update
    presetBc.callWithDelay(60, [presetName], function(name)
    {
        delayedResult = "loaded:" + name;
    });
});

presetBc.sendSyncMessage(["MyPreset"]);
```
```json:testMetadata:deferred-action-after-event
{
  "testable": false,
  "skipReason": "callWithDelay causes HISE Debug crash during validation - suspected debug-only assertion in DelayedFunction timer callback"
}
```

`callWithDelay` is independent of the broadcaster's listener system - listeners registered via `addListener` do not receive anything from it. It is a standalone timer-based function call that shares the broadcaster's bypass state.
