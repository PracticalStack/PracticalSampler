## connectToScript

**Examples:**

```javascript:per-channel-script-linking
// Title: Per-channel external script linking in a loop
// Context: When building N channels that each need a MIDI script processor
// (e.g. for arpeggio, loop control, or note processing), create a
// ScriptProcessor for each channel and connect it to a shared .js file.
// Each instance gets its own state but shares the same script logic.

const var NUM_CHANNELS = 4;
const var b = Synth.createBuilder();
b.clear();

for (i = 0; i < NUM_CHANNELS; i++)
{
    var container = b.create(b.SoundGenerators.SynthChain,
        "Container " + (i + 1), 0, b.ChainIndexes.Direct);
    var synth = b.create(b.SoundGenerators.SilentSynth,
        "Channel " + (i + 1), container, b.ChainIndexes.Direct);

    // Create a ScriptProcessor and connect it to the shared script
    var scriptProc = b.create(b.MidiProcessors.ScriptProcessor,
        "MidiScript " + (i + 1), synth, b.ChainIndexes.Midi);

    b.connectToScript(scriptProc,
        "{PROJECT_FOLDER}ConnectedScripts/ChannelProcessor.js");
}

b.flush();
```
```json:testMetadata:per-channel-script-linking
{
  "testable": false,
  "skipReason": "Requires external script file (ConnectedScripts/ChannelProcessor.js)"
}
```
