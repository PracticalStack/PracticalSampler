## connectToModuleParameter

**Examples:**

```javascript:connecting-a-cable-to
// Title: Connecting a cable to a gain module with smoothing
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
const var gainIndex = builder.create(builder.Effects.SimpleGain, "SimpleGain1", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

// Title: Connecting a cable to a gain module with smoothing
// Context: A cable drives a gain parameter directly, bypassing
// script callbacks entirely. The target range JSON maps the
// normalised 0..1 cable value to a dB range with smoothing
// to avoid zipper noise on rapid value changes.

const var rm = Engine.getGlobalRoutingManager();
const var gainCable = rm.getCable("MasterGain");

gainCable.connectToModuleParameter("SimpleGain1", "Gain", {
    "MinValue": -100.0,
    "MaxValue": 0.0,
    "SkewFactor": 5.0,
    "SmoothingTime": 50.0
});

// Now any setValue/setValueNormalised call on this cable
// automatically updates SimpleGain1's Gain parameter
gainCable.setValueNormalised(0.75);

// Get module reference for verification
const var gain = Synth.getEffect("SimpleGain1");
```
```json:testMetadata:connecting-a-cable-to
{
  "testable": true,
  "verifyScript": [
    {
      "expression": "gainCable.getValueNormalised()",
      "value": 0.75
    },
    {
      "expression": "gain.getAttribute(gain.Gain)",
      "value": -25.0
    }
  ]
}
```


```javascript:clearing-all-module-parameter
// Title: Clearing all module parameter connections
// Context: Before reconfiguring cable routing (e.g., when switching
// between plugin modes), remove all existing module connections.

const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("RoutingCable");

// Remove all module parameter connections from this cable
cable.connectToModuleParameter("", -1, {});

// Remove connections for a specific processor only
cable.connectToModuleParameter("SimpleGain1", -1, {});

```
```json:testMetadata:clearing-all-module-parameter
{
  "testable": false
}
```


**Pitfalls:**
- The `processorId` must match the module's ID exactly. If the module is not found, a script error is reported. The `parameterIndexOrId` can be either a string name or integer index -- prefer the string form for readability and resilience to parameter reordering.
