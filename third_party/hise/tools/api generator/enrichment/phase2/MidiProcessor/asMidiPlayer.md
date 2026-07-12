## asMidiPlayer

**Examples:**

```javascript:cast-for-playback-control
// Title: Casting a MidiProcessor to MidiPlayer for playback control
// Context: When you obtain a generic MidiProcessor handle (e.g. from a loop
// over all MIDI processors), cast it to MidiPlayer to access playback methods.
// The reverse cast (MidiPlayer.asMidiProcessor()) is used for state serialization.

const var mp = Synth.getMidiProcessor("MIDIPlayer1");

// Cast to MidiPlayer -- throws if the module is not a MidiPlayer
const var player = mp.asMidiPlayer();

// Now you can use MidiPlayer-specific methods
player.setPlaybackPosition(0.0);
player.play(0);

// The reverse: cast MidiPlayer back to MidiProcessor for exportState()
const var player2 = Synth.getMidiPlayer("MIDIPlayer2");
const var mp2 = player2.asMidiProcessor();
var savedState = mp2.exportState();
```
```json:testMetadata:cast-for-playback-control
{
  "testable": false,
  "skipReason": "Requires MidiPlayer modules with loaded MIDI content for meaningful playback operations"
}
```
