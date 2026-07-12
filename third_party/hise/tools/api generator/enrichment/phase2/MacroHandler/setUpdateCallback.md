## setUpdateCallback

**Examples:**

```javascript:broadcaster-macro-callback
// Title: Using a Broadcaster as the macro update callback
// Context: Instead of a plain function, pass a Broadcaster to fan out
// macro change notifications to multiple independent listeners

const var mh = Engine.createMacroHandler();

// Create a broadcaster that will receive macro change events
const var macroBroadcaster = Engine.createBroadcaster({
    "id": "macroBroadcaster",
    "args": ["macroData"]
});

// The broadcaster itself becomes the callback - when macro connections
// change, it fires as a broadcaster message that any number of listeners
// can subscribe to
mh.setUpdateCallback(macroBroadcaster);

var listenerCount = 0;

// Now multiple independent listeners can react to macro changes
macroBroadcaster.addListener("", "update macro indicators", function(macroData)
{
    // Update UI indicators showing which macros are connected
    listenerCount++;
    Console.print("Active connections: " + macroData.length);
});

macroBroadcaster.addListener("", "sync context menu state", function(macroData)
{
    // Update context menu enabled/checked states
    for (item in macroData)
        Console.print("Slot " + item.MacroIndex + ": " + item.Attribute);
});

// --- test-only ---
// Trigger listeners by forcing a macro update notification
mh.setMacroDataFromObject([]);
// --- end test-only ---
```
```json:testMetadata:broadcaster-macro-callback
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "listenerCount >= 1", "value": true}
  ]
}
```

**Pitfalls:**
- The callback fires immediately on registration with the current macro state. If you set up the MacroHandler during `onInit`, the callback body runs synchronously before the next line of init code executes. Place any code that depends on full initialization after the `setUpdateCallback` call.
