MidiPlayer::setTimeSignatureToSequence(Integer index, JSON timeSignatureObject) -> Integer

Thread safety: UNSAFE -- Modifies sequence length, may use undo manager.
Sets the time signature and length of the sequence at the given one-based index. Returns true if values are valid, false if sequence doesn't exist or any of Nominator/Denominator/NumBars <= 0.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setTimeSignatureToSequence(1, {"Nominator": 4, "Denominator": 4, "NumBars": 4});
```
Dispatch/mechanics: Parses Nominator, Denominator, NumBars, LoopStart, LoopEnd from JSON. Validates all > 0. Calls HiseMidiSequence::setLengthFromTimeSignature() if valid. Tempo property is NOT consumed.
Pair with: MidiPlayer.getTimeSignatureFromSequence -- read back. MidiPlayer.setTimeSignature -- shorthand for current sequence.
Source:
  ScriptingApiObjects.cpp:6250  setTimeSignatureToSequence() -> getSequenceWithIndex() -> setLengthFromTimeSignature()
