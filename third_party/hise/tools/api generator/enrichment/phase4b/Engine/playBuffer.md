Engine::playBuffer(var bufferData, var callback, double fileSampleRate) -> undefined

Thread safety: UNSAFE -- creates PreviewHandler lazily, spawns Job with SimpleTimer
Previews audio buffer through main output with progress callback. Mono buffers
auto-duplicated to stereo. fileSampleRate <= 0 uses engine sample rate.
Callback signature: callback(bool isPlaying, double position)
Anti-patterns:
  - Calling playBuffer again stops the current preview immediately
  - Mono buffers are always duplicated to stereo -- no true mono preview
Pair with:
  loadAudioFileIntoBufferArray -- load buffer data to preview
  renderAudio -- offline rendering alternative
Source:
  ScriptingApi.cpp  Engine::playBuffer()
    -> PreviewHandler (lazy create) -> new Job -> SimpleTimer for callbacks
