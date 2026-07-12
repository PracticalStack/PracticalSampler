## setUpdateCallback

**Examples:**

```javascript:broadcaster-fan-out
// Title: Forward automation changes to a Broadcaster for fan-out
// Context: Multiple UI systems need to react when CC assignments change
// (indicator positioning, knob enable-state, sequencer refresh).
// Instead of handling everything in one callback, forward the data
// to a Broadcaster and let each system subscribe independently.

const var mah = Engine.createMidiAutomationHandler();

const var automationBroadcaster = Engine.createBroadcaster({
    "id": "AutomationChanges",
    "args": ["data"]
});

// The update callback forwards the automation data to the broadcaster.
// It fires immediately on registration with the current state, then
// again whenever assignments change (MIDI learn, removal, preset load).
mah.setUpdateCallback(function(unused)
{
    automationBroadcaster.data = this.getAutomationDataObject();
});

// UI systems subscribe to the broadcaster independently
automationBroadcaster.addListener("", "log automation changes",
    function(data)
    {
        Console.print("CC assignments: " + data.length);
    });
```
```json:testMetadata:broadcaster-fan-out
{
  "testable": false,
  "skipReason": "setUpdateCallback fires synchronously on registration, but the Broadcaster listener is added after registration so it misses the initial fire. Subsequent fires require actual MIDI learn or preset load events that cannot be triggered programmatically."
}
```

Note that the callback fires immediately during registration with the current automation state. If your callback depends on other systems being initialized first, make sure to call `setUpdateCallback()` after those systems are ready.

**Pitfalls:**
- The `setUpdateCallback` function's parameter signature is `function(automationData)`, but using `this.getAutomationDataObject()` inside the callback (as shown above) is equivalent and lets you use the callback parameter slot for other purposes. The `this` context inside the callback refers to the MidiAutomationHandler instance.
