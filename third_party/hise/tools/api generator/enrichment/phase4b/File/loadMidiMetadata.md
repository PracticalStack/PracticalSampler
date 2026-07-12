File::loadMidiMetadata() -> JSON

Thread safety: UNSAFE -- reads MIDI file from disk and parses header (I/O).
Returns only the time signature metadata without loading individual MIDI events.
Lightweight alternative to loadAsMidiFile for tempo/time-sig queries.
Returns undefined if file does not exist or is not valid MIDI.

Example return value (canonical TimeSignature object):
  {
    "Nominator": 4.0, "Denominator": 4.0, "NumBars": 8.0,
    "Tempo": 120.0,
    "LoopStart": 0.0, "LoopEnd": 1.0  // normalised 0.0 - 1.0
  }

Pair with:
  loadAsMidiFile -- for full event loading
  writeMidiFile -- for writing MIDI with metadata

Anti-patterns:
  - Unlike loadAsMidiFile, does NOT check file extension -- attempts to parse any
    file as MIDI. Silently returns undefined on failure.

Source:
  ScriptingApiObjects.cpp  ScriptFile::loadMidiMetadata()
    -> HiseMidiSequence creation -> getTimeSignature() -> JSON conversion
