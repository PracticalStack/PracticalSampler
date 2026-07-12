Synth::setFixNoteOnAfterNoteOff(Integer shouldBeFixed) -> undefined

Thread safety: SAFE -- sets a boolean flag on the MidiProcessorChain, no allocations, no locks.
Enables or disables the attached note buffer. Must be called before using attachNote().
Typically called once in onInit with value true.

Pair with:
  attachNote -- requires this to be enabled first

Source:
  ScriptingApi.cpp  Synth::setFixNoteOnAfterNoteOff()
    -> parentMidiProcessor -> chain -> setFixNoteOnAfterNoteOff(shouldBeFixed)
