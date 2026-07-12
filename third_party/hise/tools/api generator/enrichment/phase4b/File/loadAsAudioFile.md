File::loadAsAudioFile() -> NotUndefined

Thread safety: UNSAFE -- reads entire audio file from disk into memory (I/O plus format decoding).
Reads the audio file and returns sample data as Buffer objects. Mono files return
a single Buffer; multi-channel files return an Array of Buffers (one per channel).
Supports WAV, AIFF, FLAC, OGG, and HLAC formats.

Dispatch/mechanics:
  hlac::CompressionHelpers::loadFile(f, unused)
    -> AudioFormatManager::registerBasicFormats() + HLAC
    -> reads entire file into AudioSampleBuffer
  Mono: returns single VariantBuffer
  Multi-channel: returns Array of VariantBuffer objects

Pair with:
  writeAudioFile -- to write audio data back to disk
  loadAudioMetadata -- to inspect format without loading samples

Anti-patterns:
  - Return type varies by channel count: single Buffer for mono, Array of Buffers
    for stereo+. Code that always indexes result[0] fails for mono. Normalize with:
    if (!Array.isArray(audio)) audio = [audio];
  - Reads entire file into memory. For streaming playback, use the sampler module.

Source:
  ScriptingApiObjects.cpp  ScriptFile::loadAsAudioFile()
    -> hlac::CompressionHelpers::loadFile(f, unused)
    -> reports "No valid audio file" on failure
