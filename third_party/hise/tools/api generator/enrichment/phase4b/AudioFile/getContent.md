AudioFile::getContent() -> Array

Thread safety: UNSAFE -- creates Array and Buffer objects (heap allocation).
Returns audio data as an Array of Buffer objects, one per channel. Returns the
current range data (as set by setRange), not the full original file content.
Returns empty array if no audio is loaded.

Dispatch/mechanics:
  getBuffer()->getChannelBuffer(i, false) for each channel
    -> false = current range data, not full content
    -> wraps each channel's float* as a script Buffer object

Pair with:
  setRange -- controls which portion of audio getContent returns
  update -- call after modifying returned Buffers to notify listeners

Anti-patterns:
  - Do NOT assume returned buffers contain full file data -- after setRange(1000, 2000),
    buffers contain 1000 samples. Use setRange(0, getTotalLengthInSamples()) first for full content.

Source:
  ScriptingApiObjects.cpp:1752  ScriptAudioFile::getContent()
    -> iterates buffer->getBuffer().getNumChannels()
    -> buffer->getChannelBuffer(i, false) per channel
