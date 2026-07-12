Engine::getFrequencyForMidiNoteNumber(int midiNumber) -> Double

Thread safety: SAFE -- pure inline math
Converts MIDI note number to frequency (Hz) using A440 tuning.
Note 69 -> 440.0 Hz. Formula: 440 * pow(2, (note - 69) / 12.0).
Source:
  ScriptingApi.h  inline -> MidiMessage::getMidiNoteInHertz(midiNumber)
