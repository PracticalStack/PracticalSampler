MidiPlayer::getTimeSignatureFromSequence(Integer index) -> JSON

Thread safety: UNSAFE -- Constructs a DynamicObject on the heap.
Returns the time signature of the sequence at the given one-based index as a JSON object. Returns an empty var if the sequence doesn't exist. See getTimeSignature() for the property format.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var sig = mp.getTimeSignatureFromSequence(1);
```
Pair with: MidiPlayer.setTimeSignatureToSequence -- set time signature on a specific sequence. MidiPlayer.getTimeSignature -- shorthand for current sequence.
Source:
  ScriptingApiObjects.cpp:6250  getTimeSignatureFromSequence() -> MidiPlayer::getSequenceWithIndex() -> HiseMidiSequence::TimeSignature::getAsJSON()
