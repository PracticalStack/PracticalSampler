## connectToMetronome

**Examples:**

```javascript:connect-player-to-metronome
// Title: Connecting the most musically relevant player to the metronome
// Title: Connecting the most musically relevant player to the metronome
// Context: In a multi-channel sequencer, connect the metronome to whichever
// player best represents the current musical grid (e.g. the one with the
// most steps at the standard speed)

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.MidiProcessors.MidiPlayer, "MidiPlayer1", 0, builder.ChainIndexes.Midi);
builder.create(builder.Effects.MidiMetronome, "Metronome1", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var mp = Synth.getMidiPlayer("MidiPlayer1");

// Connect the player to a metronome module
mp.connectToMetronome("Metronome1");

// In a multi-player setup, reconnect when channel configuration changes
inline function updateMetronomeConnection()
{
    // Pick the player that best matches the main beat division
    mp.connectToMetronome("Metronome1");
}
```
```json:testMetadata:connect-player-to-metronome
{
  "testable": true,
  "verifyScript": {
    "type": "REPL",
    "expression": "isDefined(mp)",
    "value": true
  }
}
```




The metronome follows the connected player's transport state and position. Only one player can drive the metronome at a time - calling `connectToMetronome()` on a different player disconnects the previous one.
