MidiPlayer::setTimeSignature(JSON timeSignatureObject) -> Integer

Thread safety: UNSAFE -- Modifies sequence length, may use undo manager.
Sets the time signature and length of the current sequence. Accepts a JSON
object in the same format as File.loadMidiMetadata (Nominator, Denominator,
NumBars required; LoopStart, LoopEnd optional). Returns true if values are
valid. Internally delegates to setTimeSignatureToSequence(-1, ...).
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setTimeSignature({"Nominator": 3, "Denominator": 4, "NumBars": 8});
```
Pair with: MidiPlayer.getTimeSignature -- read back the time signature. MidiPlayer.setTimeSignatureToSequence -- set on a specific sequence.
Anti-patterns: The Tempo property from getTimeSignature() is ignored when setting. Tempo is derived from the host/master clock, not the sequence metadata.
Source:
  ScriptingApiObjects.cpp:6250  setTimeSignature() -> setTimeSignatureToSequence(-1) -> HiseMidiSequence::setLengthFromTimeSignature()
