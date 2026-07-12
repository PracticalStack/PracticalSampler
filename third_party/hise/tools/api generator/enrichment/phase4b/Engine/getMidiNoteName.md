Engine::getMidiNoteName(int midiNumber) -> String

Thread safety: WARNING -- string construction
Converts MIDI note number to name string. Note 60 = "C3".
Source:
  ScriptingApi.h  inline
    -> MidiMessage::getMidiNoteName(midiNumber, true, true, 3)
