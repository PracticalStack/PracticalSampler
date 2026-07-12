## create

**Examples:**

```javascript:loop-channel-strip
// Title: Loop-based channel strip construction
// Context: Building N identical processing channels in a loop. Each channel
// gets a container synth, a sound generator, MIDI processors, and effects.
// This is the Builder's primary advantage over manual tree construction.

const var NUM_CHANNELS = 4;
const var b = Synth.createBuilder();

inline function buildChannel(channelIndex)
{
    // Container for this channel (Direct = add as sound generator)
    local container = b.create(b.SoundGenerators.SynthChain,
        "Container " + (channelIndex + 1), 0, b.ChainIndexes.Direct);

    // Sound generator inside the container
    local synth = b.create(b.SoundGenerators.SilentSynth,
        "Channel " + (channelIndex + 1), container, b.ChainIndexes.Direct);

    // MIDI processor on the synth
    local muter = b.create(b.MidiProcessors.MidiMuter,
        "Muter " + (channelIndex + 1), synth, b.ChainIndexes.Midi);

    // Effects on the container
    local eq = b.create(b.Effects.HardcodedMasterFX,
        "EQ " + (channelIndex + 1), container, b.ChainIndexes.FX);

    local gain = b.create(b.Effects.SimpleGain,
        "Gain " + (channelIndex + 1), container, b.ChainIndexes.FX);
}

b.clear();

for (i = 0; i < NUM_CHANNELS; i++)
    buildChannel(i);

b.flush();

// After building, retrieve modules by name:
Console.print(Synth.getChildSynth("Channel 1").getId());
Console.print(Synth.getEffect("Gain 1").getId());
Console.print(Synth.getChildSynth("Container 4").getId());
```
```json:testMetadata:loop-channel-strip
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["Channel 1", "Gain 1", "Container 4"]}
}
```

```javascript:conditional-build-flags
// Title: Conditional build flags for selective rebuilding
// Context: Large Builder scripts benefit from boolean flags that enable/disable
// subsections. This lets you iterate on one part of the tree without rebuilding
// everything, which speeds up development significantly.

const var BUILD_CHANNELS = 1;
const var BUILD_MASTER_FX = 1;
const var BUILD_SENDS = 0; // Disabled while working on channels

const var b = Synth.createBuilder();
b.clear();

if (BUILD_CHANNELS)
{
    for (i = 0; i < NUM_CHANNELS; i++)
        buildChannel(i);
}

if (BUILD_MASTER_FX)
{
    b.create(b.Effects.CurveEq, "MasterEQ", 0, b.ChainIndexes.FX);
    b.create(b.Effects.SimpleGain, "MasterGain", 0, b.ChainIndexes.FX);
}

if (BUILD_SENDS)
    buildSendEffects();

b.flush();
```
```json:testMetadata:conditional-build-flags
{
  "testable": false,
  "skipReason": "References undefined helper functions (buildChannel, buildSendEffects) and constants (NUM_CHANNELS) from a separate code context"
}
```
