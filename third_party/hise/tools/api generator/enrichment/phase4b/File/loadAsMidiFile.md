File::loadAsMidiFile(Integer trackIndex) -> JSON

Thread safety: UNSAFE -- reads MIDI file from disk and parses content (I/O plus MIDI parsing).
Reads a MIDI file and returns a JSON object with TimeSignature metadata and Events
array (MessageHolder objects) for the specified zero-based track index.
Only processes files with the .mid extension.

Example return value:
  { "TimeSignature": { ... }, "Events": [ ... ] }
  TimeSignature sub-object: see File.loadMidiMetadata for format.
  Events: Array of MessageHolder objects.

Dispatch/mechanics:
  Checks file extension == ".mid" (rejects .midi, .smf etc.)
  -> creates HiseMidiSequence from file
  -> setCurrentTrackIndex(trackIndex)
  -> getEventList(44100.0, 120.0) converts to MessageHolder objects
  Uses HISE internal resolution of 960 ticks per quarter note.

Pair with:
  writeMidiFile -- to write MIDI data back to disk
  loadMidiMetadata -- for lightweight metadata-only loading

Anti-patterns:
  - [BUG] Only processes .mid extension. Files with .midi or .smf extensions are
    silently ignored, returning empty with no error.
  - Events use fixed sample rate 44100 Hz and tempo 120 BPM for timestamp
    conversion. Actual playback rate may differ.

Source:
  ScriptingApiObjects.cpp  ScriptFile::loadAsMidiFile()
    -> HiseMidiSequence creation -> setCurrentTrackIndex -> getEventList(44100, 120)
