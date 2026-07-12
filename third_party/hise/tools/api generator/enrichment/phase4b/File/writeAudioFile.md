File::writeAudioFile(var audioData, Double sampleRate, Integer bitDepth) -> Integer

Thread safety: UNSAFE -- performs filesystem I/O (audio encoding and writing). May allocate for buffered paths.
Writes audio data to this file in the format determined by file extension.
Supports WAV, AIFF, FLAC, OGG output. Returns true on success.

Accepted audioData shapes:
  Single Buffer         -- mono, no buffering
  Array of Buffers      -- multi-channel, no buffering
  Array of number arrays -- multi-channel, buffered (float sanitized)
  Plain number array    -- mono, buffered (float sanitized)

Dispatch/mechanics:
  AudioFormatManager::registerBasicFormats()
  -> findFormatForFileExtension(f.getFileExtension())
  -> deletes existing file, creates writer with quality=9
  -> writes samples per input shape (Buffer direct or buffered copy)
  Multi-channel: validates all channels have same sample count.

Pair with:
  loadAsAudioFile -- to read audio data from disk
  loadAudioMetadata -- to inspect format without full loading

Anti-patterns:
  - [BUG] Existing file is deleted before writing begins. If write fails (e.g.,
    invalid bit depth), the original file is lost.
  - HLAC is NOT supported for writing (only for reading via loadAsAudioFile).
  - Output format is determined by file extension only. Unrecognized extensions
    (e.g., .mp3) report a script error.
  - Buffered paths (number arrays) sanitize NaN/Inf to zero silently. Buffer paths
    do not sanitize.
  - Multi-channel size mismatch reports "Size mismatch at channel N".

Source:
  ScriptingApiObjects.cpp  ScriptFile::writeAudioFile()
    -> AudioFormatManager::findFormatForFileExtension
    -> createWriterFor(fileOutputStream, sampleRate, numChannels, bitDepth, {}, 9)
