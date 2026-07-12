MidiPlayer::isEmpty() -> Integer

Thread safety: SAFE
Returns true if the player has no valid sequence loaded (player reference is null or no current sequence exists). Checks for sequence presence, not whether the sequence contains MIDI data -- use isSequenceEmpty() for that.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var empty = mp.isEmpty();
```
Pair with: MidiPlayer.isSequenceEmpty -- checks whether a specific sequence contains MIDI events.
Source:
  ScriptingApiObjects.cpp:6250  isEmpty() -> !sequenceValid() -> checks getPlayer() != nullptr && getSequence() != nullptr
