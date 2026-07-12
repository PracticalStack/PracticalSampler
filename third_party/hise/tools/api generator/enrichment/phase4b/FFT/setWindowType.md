FFT::setWindowType(Integer windowType) -> undefined

Thread safety: UNSAFE -- triggers reinitialise() which reallocates all work buffers and recomputes the window function under a write lock
Sets the window function applied to each chunk before forward FFT. Use the
constants on the FFT object: Rectangle (0), Triangle (1), Hamming (2), Hann (3),
BlackmanHarris (4), Kaiser (5), FlatTop (6).

Dispatch/mechanics:
  Sets currentWindowType, then reinitialise() -> prepare(lastSpecs)
  prepare() creates windowBuffer (2x FFT size) and applies FFTHelpers::applyWindow()

Pair with:
  setOverlap -- window function and overlap work together for spectral quality
  prepare -- recomputes window buffer (set window type before prepare to avoid double allocation)

Source:
  ScriptingApiObjects.cpp  ScriptFFT::setWindowType()
    -> currentWindowType = (WindowType)windowType
    -> reinitialise() -> prepare() -> FFTHelpers::applyWindow(currentWindowType, windowBuffer)
