MidiPlayer::asMidiProcessor() -> ScriptObject

Thread safety: UNSAFE -- Creates a new ScriptingMidiProcessor wrapper object (heap allocation).
Returns a typed MidiProcessor reference for this MIDI Player module, enabling access to generic MidiProcessor methods like setAttribute() and getAttribute() for module parameters.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var proc = mp.asMidiProcessor();
```
Pair with: MidiProcessor.setAttribute, MidiProcessor.getAttribute -- for controlling module parameters (CurrentPosition, CurrentSequence, CurrentTrack, LoopEnabled, LoopStart, LoopEnd, PlaybackSpeed).
Source:
  ScriptingApiObjects.cpp:6250  asMidiProcessor() -> new ScriptingMidiProcessor(getScriptProcessor(), p)
