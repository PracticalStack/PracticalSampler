File::loadAudioMetadata() -> JSON

Thread safety: UNSAFE -- reads file header from disk via AudioFormatReader (I/O).
Reads the audio file's header information without loading full sample data.
Returns a JSON object with format details and a nested Metadata object.
Returns undefined silently if the file does not exist or is not a recognized format.

Example return value:
  {
    "SampleRate": 44100.0, "NumChannels": 2, "NumSamples": 132300,
    "BitDepth": 24, "Format": "WAV",
    "File": "C:/samples/recording.wav",
    "Metadata": {}  // format-specific tags (BWF, ID3, etc.)
  }

Pair with:
  loadAsAudioFile -- to load actual sample data
  loadMidiMetadata -- analogous metadata loading for MIDI

Anti-patterns:
  - Returns undefined silently on failure (no error reported). Always check with
    isDefined() before accessing properties.

Source:
  ScriptingApiObjects.cpp  ScriptFile::loadAudioMetadata()
    -> AudioFormatManager::registerBasicFormats() -> createReaderFor(f)
    -> extracts reader properties into JSON object
