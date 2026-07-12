## sendData

**Examples:**

```javascript
// Title: Sending structured state data between script processors
// Context: One script processor collects the current state of a
// multi-parameter control (e.g., an envelope editor) and sends
// it as a JSON object through a cable. Another script processor
// receives it via registerDataCallback to update its UI.

const var rm = Engine.getGlobalRoutingManager();
const var stateCable = rm.getCable("ControlState");

// Build a state snapshot and send it through the data channel
inline function broadcastState()
{
    local state = {
        "attack": attackKnob.getValue(),
        "decay": decayKnob.getValue(),
        "sustain": sustainKnob.getValue(),
        "mode": modeSelector.getValue()
    };

    stateCable.sendData(state);
};
```

**Pitfalls:**
- Do not call `sendData()` from inside a synchronous cable callback or any audio-thread context. The method allocates a `MemoryOutputStream` on the heap, which is not realtime-safe. Use a Timer or an async callback to defer the call if needed.
