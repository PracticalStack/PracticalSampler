MidiPlayer::setSequence(Integer sequenceIndex) -> undefined

Thread safety: UNSAFE -- Calls setAttribute with sendNotificationAsync.
Selects one of the previously loaded sequences as the current active sequence. Uses one-based indexing.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setSequence(1);
```
Pair with: MidiPlayer.getNumSequences -- get count to validate index. MidiPlayer.setFile -- load sequences first.
Anti-patterns: Index must be >= 1. Sequence and track indices are one-based; index 0 triggers a script error.
Source:
  ScriptingApiObjects.cpp:6250  setSequence() -> MidiPlayer::setAttribute(CurrentSequence, sendNotificationAsync)
