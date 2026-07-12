FFT::prepare(Integer powerOfTwoSize, Integer maxNumChannels) -> undefined

Thread safety: UNSAFE -- allocates audio buffers, per-channel work buffers, and creates the JUCE FFT engine instance under a write lock
Allocates all internal buffers required for FFT processing. Must be called before
process(). Work buffers are allocated conditionally based on current callback and
inverse FFT settings.

Required setup:
  const var fft = Engine.createFFT();
  fft.setWindowType(fft.Hann);
  fft.setMagnitudeFunction(onMagnitude, false);
  // Configure callbacks BEFORE prepare to avoid double allocation
  fft.prepare(1024, 1);

Dispatch/mechanics:
  Validates isPowerOfTwo(size), clamps channels to [1, 16]
  Creates window buffer (2x FFT size), applies window function
  Allocates per-channel WorkBuffers: chunkInput always, chunkOutput/magBuffer/phaseBuffer conditionally
  Creates juce::dsp::FFT(order, useFallback) under ScopedWriteLock

Pair with:
  process -- runs the FFT pipeline on audio data (requires prepare first)
  setMagnitudeFunction/setPhaseFunction -- set callbacks before prepare to avoid double allocation

Anti-patterns:
  - Do NOT pass a non-power-of-two size -- throws a script error
  - Do NOT call prepare() before configuring callbacks -- changing callbacks after prepare
    triggers reinitialise() which re-allocates all buffers (unnecessary double allocation)

Source:
  ScriptingApiObjects.cpp:8271  ScriptFFT::prepare()
    -> validates isPowerOfTwo, clamps channels jlimit(1, NUM_MAX_CHANNELS)
    -> creates windowBuffer, applies FFTHelpers::applyWindow()
    -> allocates WorkBuffer per channel (chunkInput, chunkOutput, magBuffer, phaseBuffer)
    -> new juce::dsp::FFT(order, useFallback) under ScopedWriteLock
