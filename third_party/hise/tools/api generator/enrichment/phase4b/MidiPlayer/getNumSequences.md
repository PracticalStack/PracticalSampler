MidiPlayer::getNumSequences() -> Integer

Thread safety: SAFE
Returns the number of MIDI sequences currently loaded in this player. Returns 0 if no player reference exists.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var num = mp.getNumSequences();
```
Pair with: MidiPlayer.setSequence -- select a sequence by index. MidiPlayer.setFile -- load sequences.
Source:
  ScriptingApiObjects.cpp:6250  getNumSequences() -> MidiPlayer::currentSequences.size()
