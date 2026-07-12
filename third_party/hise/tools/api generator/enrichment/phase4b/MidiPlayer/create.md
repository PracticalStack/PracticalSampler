MidiPlayer::create(Integer nominator, Integer denominator, Integer barLength) -> undefined

Thread safety: UNSAFE -- Allocates a new HiseMidiSequence and adds it to the sequence list.
Creates a new empty MIDI sequence with the given time signature and bar count, appends it to the player's sequence list, and selects it as current. Does NOT clear existing sequences -- call clearAllSequences() first to replace them.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.clearAllSequences();
mp.create(4, 4, 4);
```
Dispatch/mechanics: Constructs a HiseMidiSequence, sets time signature (nominator/denominator/barLength), creates one empty track, then calls MidiPlayer::addSequence() which appends and selects the new sequence.
Pair with: MidiPlayer.clearAllSequences -- clear before creating if replacing. MidiPlayer.setFile -- alternative way to populate with a MIDI file.
Source:
  ScriptingApiObjects.cpp:6250  create() -> new HiseMidiSequence() -> setLengthFromTimeSignature() -> createEmptyTrack() -> MidiPlayer::addSequence()
