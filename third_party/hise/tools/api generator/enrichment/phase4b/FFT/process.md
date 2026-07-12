FFT::process(AudioData dataToProcess) -> AudioData

Thread safety: UNSAFE -- allocates output buffers, Spectrum2D objects, and spectrum images. Acquires a read lock for the callback processing loop.
Runs the FFT processing pipeline on input audio data. Accepts a single Buffer
(mono) or Array of Buffers (multi-channel). Returns reconstructed audio when
inverse FFT is enabled, undefined otherwise.

Required setup:
  const var fft = Engine.createFFT();
  fft.setWindowType(fft.Hann);
  fft.setOverlap(0.5);
  fft.setMagnitudeFunction(onMagnitude, false);
  fft.prepare(1024, 1);
  var result = fft.process(inputBuffer);

Dispatch/mechanics:
  Two independent code paths (can both be active):
  1. Spectrum2D path (if enabled): generates spectrogram image before callbacks
  2. Callback path: steps through input in chunks of fftSize*(1-overlap),
     each chunk windowed -> forward FFT -> mag/phase callbacks -> optional inverse FFT with overlap-add
  First chunk skips first quarter of window to avoid startup edge artifacts.

Pair with:
  prepare -- must be called before process (throws error otherwise)
  setMagnitudeFunction/setPhaseFunction -- callbacks invoked per chunk
  setEnableInverseFFT -- enables time-domain reconstruction and return value

Anti-patterns:
  - Do NOT call without prepare() -- throws "You must call prepare before process"
  - Do NOT call without any callbacks or Spectrum2D enabled -- throws "the process function is not defined"

Source:
  ScriptingApiObjects.cpp:8471  ScriptFFT::process()
    -> Spectrum2D path: new Spectrum2D(input) -> generates Image
    -> Callback path: iterates chunks with numDelta = maxNumSamples * (1.0 - overlap)
       -> copyToWorkBuffer() -> applyFFT() -> magnitudeFunction.callSync() -> phaseFunction.callSync()
       -> applyInverseFFT() -> copyFromWorkBuffer() (overlap-add)
