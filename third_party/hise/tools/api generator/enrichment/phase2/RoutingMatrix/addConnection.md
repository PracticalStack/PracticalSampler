## addConnection

**Examples:**

```javascript:multi-output-bus-routing
// Title: Multi-output bus routing with host channel detection
// Context: Route a stereo source to a user-selected DAW output pair,
// falling back to stereo if the host doesn't support the channel count.

const var rm = Synth.getRoutingMatrix("MySynth");

inline function onOutputSelectorControl(component, value)
{
    local outputPair = parseInt(value) - 1;
    local destL = outputPair * 2;
    local destR = outputPair * 2 + 1;

    // addConnection returns false if the destination channel is unavailable
    rm.addConnection(0, destL);
    local ok = rm.addConnection(1, destR);

    if (!ok)
    {
        // Host doesn't support this channel count -- fall back to stereo
        rm.addConnection(0, 0);
        rm.addConnection(1, 1);
    }
}

const var outputSelector = Content.getComponent("OutputSelector");
outputSelector.setControlCallback(onOutputSelectorControl);
```
```json:testMetadata:multi-output-bus-routing
{
  "testable": false,
  "skipReason": "Requires named processor 'MySynth' and multichannel host output configuration"
}
```

```javascript:dynamic-per-channel-output
// Title: Dynamic per-channel output assignment in a multichannel instrument
// Context: Each channel strip routes its stereo output to a selectable
// DAW output pair. A master matrix passes all channels through when
// multi-output is enabled, or folds everything to stereo when disabled.

const var NUM_CHANNELS = 4;
const var masterMatrix = Synth.getRoutingMatrix("MasterSynth");
const var channelMatrices = [];

for (i = 0; i < NUM_CHANNELS; i++)
    channelMatrices.push(Synth.getRoutingMatrix("Channel " + (i + 1)));

// Expand master matrix for multi-output
masterMatrix.setNumChannels(16);

inline function enableMultiOutput(enable)
{
    masterMatrix.clear();

    if (enable)
    {
        // Pass all channel pairs through to separate DAW outputs
        for (i = 0; i < 16; i++)
        {
            masterMatrix.addConnection(i * 2, i * 2);
            masterMatrix.addConnection(i * 2 + 1, i * 2 + 1);
        }
    }
    else
    {
        // Fold everything to stereo
        masterMatrix.addConnection(0, 0);
        masterMatrix.addConnection(1, 1);
    }
}

inline function setChannelOutput(channelIndex, outputPair)
{
    local matrix = channelMatrices[channelIndex];
    matrix.addConnection(0, outputPair * 2);
    matrix.addConnection(1, outputPair * 2 + 1);
}
```
```json:testMetadata:dynamic-per-channel-output
{
  "testable": false,
  "skipReason": "Requires multiple named synth processors (MasterSynth, Channel 1-4) with multichannel configuration"
}
```

```javascript:builder-effect-channel-isolation
// Title: Builder API -- routing effect processors to specific channel pairs
// Context: When constructing a multichannel module tree with the Builder API,
// each effect is isolated to process only its designated stereo pair within
// a 6-channel internal bus.

const var b = Synth.createBuilder();

// Create a synth with 6 internal channels (3 stereo layers)
local synth = b.create(b.SoundGenerators.SilentSynth, "MySynth", 0, b.ChainIndexes.Direct);
local synthMatrix = b.get(synth, b.InterfaceTypes.RoutingMatrix);
synthMatrix.setNumChannels(6);

// Mix channels 2-5 down to the main stereo pair
synthMatrix.addConnection(2, 0);
synthMatrix.addConnection(3, 1);
synthMatrix.addConnection(4, 0);
synthMatrix.addConnection(5, 1);

// Create effects that each process only one stereo pair
local labels = ["A", "B", "C"];

for (i = 0; i < 3; i++)
{
    local fx = b.create(b.Effects.HardcodedPolyphonicFX, "FX_" + labels[i], synth, b.ChainIndexes.FX);
    local fxMatrix = b.get(fx, b.InterfaceTypes.RoutingMatrix);

    // Isolate this effect to its stereo pair
    fxMatrix.clear();
    fxMatrix.addConnection(i * 2, i * 2);
    fxMatrix.addConnection(i * 2 + 1, i * 2 + 1);
}

b.flush();
```
```json:testMetadata:builder-effect-channel-isolation
{
  "testable": false,
  "skipReason": "Requires HardcodedPolyphonicFX module type availability and multichannel internal bus propagation from parent"
}
```

**Pitfalls:**
- When routing to higher output pairs (channels 4+), always check the return value. The host DAW determines how many output channels are available -- `addConnection` silently fails (returns `false`) if the destination channel exceeds the host's channel count.
