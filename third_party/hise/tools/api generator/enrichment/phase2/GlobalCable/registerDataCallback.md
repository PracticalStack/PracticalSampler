## registerDataCallback

**Examples:**

```javascript
// Title: Receiving structured envelope data for UI rendering
// Context: A DSP network sends a JSON snapshot of an envelope's
// current state through a cable's data channel. The script uses
// this data to render an interactive AHDSR editor on a ScriptPanel.

const var rm = Engine.getGlobalRoutingManager();
const var envelopeCable = rm.getCable("EnvelopeUI");

const var EditorPanel = Content.getComponent("EnvelopeEditor");

inline function onEnvelopeData(data)
{
    // The data object contains envelope state properties sent by
    // the DSP network: Attack, Hold, Decay, Sustain, Release,
    // AttackCurve, DecayCurve, ReleaseCurve, CurrentState, etc.
    for (s in data)
        EditorPanel.data[s] = data[s];

    // Convert time values to normalised positions for drawing
    local timeSkew = 0.2;
    EditorPanel.data.attackNorm = Math.pow(data.Attack / 30000.0, timeSkew);
    EditorPanel.data.decayNorm = Math.pow(data.Decay / 30000.0, timeSkew);

    EditorPanel.repaint();
};

envelopeCable.registerDataCallback(onEnvelopeData);
```

**Pitfalls:**
- Data callbacks are always asynchronous (high-priority). Unlike value callbacks, there is no synchronous option for data.
- The recursion guard prevents a cable reference from receiving its own `sendData()` calls. This means if script A sends data through a cable and script A also has a data callback on the same cable reference, the callback will not fire. A second cable reference to the same cable name (from a different script processor) will receive the data normally.
