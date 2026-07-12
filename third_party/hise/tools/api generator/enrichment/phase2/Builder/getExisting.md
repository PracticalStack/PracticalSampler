## getExisting

**Examples:**

```javascript:multi-pass-getexisting
// Title: Multi-pass building with getExisting()
// Context: When building in phases (channels first, then send effects),
// use getExisting() to look up modules created in an earlier pass. This is
// essential for send effect architectures where per-channel sends must
// reference a shared send bus created separately.

const var NUM_CHANNELS = 4;
const var b = Synth.createBuilder();
b.clear();

// Phase 1: Build channels
for (i = 0; i < NUM_CHANNELS; i++)
{
    b.create(b.SoundGenerators.SynthChain,
        "Container " + (i + 1), 0, b.ChainIndexes.Direct);
}

// Phase 2: Create shared send bus
var sendContainer = b.create(b.SoundGenerators.SendContainer,
    "SendFX", 0, b.ChainIndexes.Direct);
b.create(b.Effects.HardcodedMasterFX,
    "Delay", sendContainer, b.ChainIndexes.FX);
b.create(b.Effects.HardcodedMasterFX,
    "Reverb", sendContainer, b.ChainIndexes.FX);

// Phase 3: Add per-channel send effects by looking up existing containers
for (i = 0; i < NUM_CHANNELS; i++)
{
    var c = b.getExisting("Container " + (i + 1));

    b.create(b.Effects.SimpleGain,
        "ChannelGain " + (i + 1), c, b.ChainIndexes.FX);
    b.create(b.Effects.SendFX,
        "SendDelay " + (i + 1), c, b.ChainIndexes.FX);
    b.create(b.Effects.SendFX,
        "SendReverb " + (i + 1), c, b.ChainIndexes.FX);
}

b.flush();

Console.print(Synth.getChildSynth("Container 1").getId());
Console.print(Synth.getEffect("ChannelGain 1").getId());
Console.print(Synth.getChildSynth("SendFX").getId());
```
```json:testMetadata:multi-pass-getexisting
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["Container 1", "ChannelGain 1", "SendFX"]}
}
```
