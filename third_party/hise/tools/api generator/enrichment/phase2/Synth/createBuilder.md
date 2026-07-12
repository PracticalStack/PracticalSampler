## createBuilder

**Examples:**

```javascript:channel-strip-builder
// Title: Programmatic channel-strip construction with the Builder API
// Context: A multi-channel instrument uses the Builder to create identical
// channel strips with MIDI players, routing matrices, and effect chains.
// The build function is commented out by default and only uncommented when
// the module tree structure needs to change.

// In onInit (commented out after initial build):
// buildModuleTree();

inline function buildModuleTree()
{
    const var b = Synth.createBuilder();
    b.clear();

    const var NUM_CHANNELS = 4;

    for (i = 0; i < NUM_CHANNELS; i++)
    {
        // Create a container synth chain for each channel
        local container = b.create(
            b.SoundGenerators.SynthChain,
            "Channel" + (i + 1), 0,
            b.ChainIndexes.Direct
        );

        // Add a silent synth as the sound generator
        local synth = b.create(
            b.SoundGenerators.SilentSynth,
            "Synth" + (i + 1), container,
            b.ChainIndexes.Direct
        );

        // Configure routing: 6 internal channels mapped to stereo pairs
        local matrix = b.get(synth, b.InterfaceTypes.RoutingMatrix);
        matrix.setNumChannels(6);
        matrix.addConnection(2, 0);
        matrix.addConnection(3, 1);

        // Add effects to the channel strip
        local fx = b.create(
            b.Effects.SimpleGain,
            "Gain" + (i + 1), synth,
            b.ChainIndexes.FX
        );

        b.setAttributes(fx, {"Gain": 0.5});
    }

    b.flush();
}
```
```json:testMetadata:channel-strip-builder
{
  "testable": false,
  "skipReason": "Builder operations modify the module tree and require a running synth context with factory types"
}
```

The Builder API follows an activate-modify-run-deactivate workflow: during development, uncomment the build call, modify the Builder script, compile once to construct the module tree, then comment it out again. The module tree persists in the project XML - the Builder is not needed at runtime. This is more maintainable than manual IDE editing for architectures with many identical channel strips.

**Cross References:**
- `Builder.create`
- `Builder.flush`
- `Builder.get`
