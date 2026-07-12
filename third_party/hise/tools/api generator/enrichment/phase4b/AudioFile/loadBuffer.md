AudioFile::loadBuffer(AudioData bufferData, Double sampleRate, Array loopRange) -> undefined

Thread safety: UNSAFE -- modifies audio buffer, acquires write lock.
Loads audio data from script Buffer objects. Accepts a single Buffer (mono) or
an Array of Buffers (multi-channel). The loop range is optional -- pass a
two-element array [start, end] to set loop points, or an empty array to skip.

Required setup:
  const var af = Engine.createAndRegisterAudioFile(0);
  const var buf = Buffer.create(128);

Dispatch/mechanics:
  bufferData.isArray() -> extracts float* from each Buffer -> AudioSampleBuffer(ptrs, numChannels, numSamples)
  bufferData.getBuffer() -> single Buffer -> buffer->loadBuffer(b->buffer, sampleRate, lr)
  Both paths -> MultiChannelAudioBuffer::loadBuffer() with write lock

Anti-patterns:
  - Do NOT pass an Array with Buffers of different lengths -- sample count is
    taken from the last valid Buffer, shorter buffers read past their end
    (undefined behavior).
  - Do NOT include non-Buffer elements in the Array -- the corresponding channel
    pointer is uninitialized, causing crashes or data corruption.

Source:
  ScriptingApiObjects.cpp:1709  ScriptAudioFile::loadBuffer()
    -> branches on bufferData.isArray() vs bufferData.getBuffer()
    -> MultiChannelAudioBuffer::loadBuffer(ab, sampleRate, lr)
