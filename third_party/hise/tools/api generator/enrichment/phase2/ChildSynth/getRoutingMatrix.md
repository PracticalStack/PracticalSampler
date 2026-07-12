## getRoutingMatrix

**Examples:**

```javascript:multi-output-routing
// Title: Multi-output routing for a child synth
// Context: A multi-output plugin routes different child synths to separate
// stereo output pairs. The routing matrix is obtained from the child synth
// and connections are configured based on a user-selectable output selector.

const var childSynth = Synth.getChildSynth("Generator1");
const var matrix = childSynth.getRoutingMatrix();

inline function onOutputSelectorControl(component, value)
{
    local outputPair = parseInt(value) - 1;
    local destL = outputPair * 2;
    local destR = outputPair * 2 + 1;

    matrix.addConnection(0, destL);

    // addConnection returns false if the output channel doesn't exist
    local success = matrix.addConnection(1, destR);

    if (!success)
    {
        // Fall back to stereo output 1+2 if the host doesn't support multichannel
        matrix.addConnection(0, 0);
        matrix.addConnection(1, 1);
    }
}
```
```json:testMetadata:multi-output-routing
{
  "testable": false,
  "skipReason": "Defines a control callback requiring UI interaction; requires a named child synth in the module tree"
}
```

**Cross References:**
- `Synth.getRoutingMatrix`
- `RoutingMatrix.addConnection`
