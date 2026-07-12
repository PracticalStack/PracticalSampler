MidiPlayer::isSequenceEmpty(Integer indexOneBased) -> Integer

Thread safety: SAFE
Returns true if the sequence at the given one-based index contains no MIDI events. Returns true if the sequence doesn't exist.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var empty = mp.isSequenceEmpty(1);
```
Pair with: MidiPlayer.isEmpty -- checks whether any sequence is loaded at all. MidiPlayer.getNumSequences -- get count of loaded sequences.
Source:
  ScriptingApiObjects.cpp:6250  isSequenceEmpty() -> MidiPlayer core check (0 events or nonexistent)
