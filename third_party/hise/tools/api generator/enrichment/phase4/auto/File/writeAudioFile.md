Writes audio data to this file. The output format is determined by the file extension: `.wav` for WAV, `.aiff` for AIFF, `.flac` for FLAC, `.ogg` for OGG. The `audioData` parameter accepts several input shapes: a single Buffer (mono), an Array of Buffers (multi-channel), an Array of number arrays, or a plain number array (mono). For multi-channel data, all channels must have the same sample count.

> [!Warning:Deletes existing file before write] The existing file is deleted before the write begins. If the write fails, the original file is lost. Use `getNonExistentSibling()` when exporting to avoid overwriting user files.

> [!Warning:Only WAV, AIFF, and FLAC supported] HLAC format is supported for reading (via `loadAsAudioFile`) but not for writing. Using an unrecognised extension like `.mp3` or `.hlac` triggers a script error.
