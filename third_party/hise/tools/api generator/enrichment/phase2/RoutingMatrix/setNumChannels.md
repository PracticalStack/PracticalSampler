## setNumChannels

**Examples:**

```javascript:expand-multi-output-routing
// Title: Expanding a synth for multi-output routing
// Context: Before configuring multichannel routing, expand the matrix
// to accommodate all needed output pairs. This also relaxes the stereo
// constraint that would otherwise auto-correct connections.

const var masterMatrix = Synth.getRoutingMatrix("MasterSynth");

// Expand to at least 16 channels, or the host's channel count if larger
masterMatrix.setNumChannels(Math.max(16, Engine.getNumPluginChannels()));

// Now multichannel addConnection calls won't be auto-corrected
masterMatrix.addConnection(0, 0);
masterMatrix.addConnection(1, 1);
masterMatrix.addConnection(2, 2);
masterMatrix.addConnection(3, 3);
// ... etc.
```
```json:testMetadata:expand-multi-output-routing
{
  "testable": false,
  "skipReason": "Requires named processor 'MasterSynth' and Engine.getNumPluginChannels() varies by host configuration"
}
```

```javascript:builder-six-channel-bus
// Title: Builder API -- creating a 6-channel internal bus
// Context: When building a channel strip with the Builder API, expand
// the internal channel count to support multiple stereo layers
// (e.g., 3 layers = 6 channels) routed through dedicated effects.

const var b = Synth.createBuilder();

local synth = b.create(b.SoundGenerators.SilentSynth, "Channel1", 0, b.ChainIndexes.Direct);
local matrix = b.get(synth, b.InterfaceTypes.RoutingMatrix);

// 3 stereo layers = 6 internal channels
matrix.setNumChannels(6);

// Mix layers 2 and 3 down to the main stereo pair
matrix.addConnection(2, 0);
matrix.addConnection(3, 1);
matrix.addConnection(4, 0);
matrix.addConnection(5, 1);

b.flush();
```
```json:testMetadata:builder-six-channel-bus
{
  "testable": false,
  "skipReason": "Builder API matrix resizing requires processor-level resizeAllowed flag; SilentSynth support is not guaranteed"
}
```

**Pitfalls:**
- `setNumChannels` sets source channels and relaxes the `numAllowedConnections` constraint, but does NOT change the destination channel count. The destination count is determined by the processor's parent context (e.g., the parent container's channel configuration).
