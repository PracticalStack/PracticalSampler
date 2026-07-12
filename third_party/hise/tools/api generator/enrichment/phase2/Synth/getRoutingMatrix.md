## getRoutingMatrix

**Examples:**

```javascript:dynamic-multi-output-routing
// Title: Dynamic multi-output routing with per-channel bus assignment
// Context: A multi-channel instrument allows each channel to be routed to a
// different stereo output pair. A master routing matrix is resized to accommodate
// all available plugin outputs, and per-channel matrices are reconfigured when
// the user selects a different output bus.

const var NUM_CHANNELS = 4;

// Get routing matrices for the master and each channel container
const var masterMatrix = Synth.getRoutingMatrix("MasterSynth");
const var channelMatrices = [];

for (i = 0; i < NUM_CHANNELS; i++)
    channelMatrices.push(Synth.getRoutingMatrix("Channel" + (i + 1)));

// Resize master to accommodate all plugin output channels
masterMatrix.setNumChannels(Math.max(16, Engine.getNumPluginChannels()));

reg multiOutEnabled = false;

// Toggle between stereo and multi-out mode
inline function onMultiOutToggle(component, value)
{
    multiOutEnabled = value;
    masterMatrix.clear();

    if (value)
    {
        // In multi-out mode, route all stereo pairs 1:1
        for (i = 0; i < 8; i++)
        {
            masterMatrix.addConnection(i * 2, i * 2);
            masterMatrix.addConnection(i * 2 + 1, i * 2 + 1);
        }
    }
    else
    {
        // In stereo mode, everything goes to outputs 1-2
        masterMatrix.addConnection(0, 0);
        masterMatrix.addConnection(1, 1);
    }

    rebuildChannelRouting();
}

// Remap a channel's stereo pair to the selected output bus
inline function rebuildChannelRouting()
{
    for (i = 0; i < NUM_CHANNELS; i++)
    {
        // outputIndex is 0-based: 0 = MASTER (1/2), 1 = 3/4, 2 = 5/6, etc.
        local outputIndex = multiOutEnabled ? (parseInt(selectors[i].getValue()) - 1) : 0;
        local matrix = channelMatrices[i];

        matrix.addConnection(0, outputIndex * 2);
        matrix.addConnection(1, outputIndex * 2 + 1);
    }
}
```
```json:testMetadata:dynamic-multi-output-routing
{
  "testable": false,
  "skipReason": "Requires a module tree with MasterSynth and Channel1-4 processors and selectors UI components"
}
```

This pattern uses `getRoutingMatrix` multiple times in `onInit` to cache references for both the master output and each channel container. The matrices are then manipulated in control callbacks to dynamically reroute audio between stereo and multi-output configurations. Note that `getRoutingMatrix` uses a global-rooted search, so the processor name must be unique across the entire module tree.
