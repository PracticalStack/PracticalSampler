## attachToOtherBroadcaster

**Examples:**

```javascript:cross-broadcaster-event-graph
// Title: Cross-broadcaster communication - forming a reactive event graph
// Context: In a complex plugin, broadcasters can target other broadcasters
// as listeners, creating a chain of reactive events. This allows event
// propagation without tight coupling between the systems that own each
// broadcaster.

// A channel selection broadcaster owned by the navigation system
const var channelBc = Engine.createBroadcaster({
    "id": "ChannelSelector",
    "args": ["channelIndex"]
});

// A filter state broadcaster owned by the effects system
const var filterBc = Engine.createBroadcaster({
    "id": "FilterUpdate",
    "args": ["channelIndex"]
});

var filterLog = [];

// When the channel changes, forward the event to the filter broadcaster
// so it can re-evaluate filter bypass states for the new channel
filterBc.attachToOtherBroadcaster(
    channelBc,
    false,  // No transform - forward args directly
    false,  // Synchronous forwarding
    "channelToFilter"
);

filterBc.addListener("", "updateFilter", function(channelIndex)
{
    filterLog.push(channelIndex);
});

channelBc.sendSyncMessage([2]); // Both broadcasters fire
```
```json:testMetadata:cross-broadcaster-event-graph
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "filterLog.length", "value": 1},
    {"type": "REPL", "expression": "filterLog[0]", "value": 2}
  ]
}
```

```javascript:argument-transformation-chain
// Title: Argument transformation between broadcasters
// Context: When chaining broadcasters with different argument structures,
// use the transform function to reshape the data.

const var source = Engine.createBroadcaster({
    "id": "KnobPair",
    "args": ["component", "value"]
});

const var target = Engine.createBroadcaster({
    "id": "ScaledValue",
    "args": ["scaledValue"]
});

var scaledLog = [];

// Transform 2 args (component, value) into 1 arg (scaled value)
inline function scaleValue(component, value)
{
    return [value * 100.0];
}

target.attachToOtherBroadcaster(source, scaleValue, false, "scale");

target.addListener("", "display", function(scaledValue)
{
    scaledLog.push(scaledValue);
});

// --- test-only ---
source.sendSyncMessage(["test", 0.75]);
// --- end test-only ---
```
```json:testMetadata:argument-transformation-chain
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "scaledLog[scaledLog.length - 1]", "value": 75.0}
  ]
}
```

**Pitfalls:**
- The transform function must return an **array** matching the target broadcaster's argument count. Returning a scalar value causes the original source arguments to be forwarded unchanged, which may trigger an argument count mismatch error.
