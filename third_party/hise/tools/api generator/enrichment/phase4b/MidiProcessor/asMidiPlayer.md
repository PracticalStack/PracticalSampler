MidiProcessor::asMidiPlayer() -> ScriptObject

Thread safety: UNSAFE -- creates a new ScriptedMidiPlayer wrapper object (heap allocation).
Casts this MidiProcessor handle to a MidiPlayer handle. Performs a runtime type
check -- succeeds only if the underlying module is a MidiPlayer, otherwise throws
a script error.
Pair with:
  MidiPlayer.asMidiProcessor -- reverse cast from MidiPlayer back to MidiProcessor
Anti-patterns:
  - Do NOT call on non-MidiPlayer modules -- throws "The module is not a MIDI player". Only use on modules known to be MidiPlayer instances.
Source:
  ScriptingApiObjects.cpp:4752  asMidiPlayer()
    -> dynamic_cast<MidiPlayer*>(mp.get())
    -> new ScriptedMidiPlayer(getScriptProcessor(), player)
