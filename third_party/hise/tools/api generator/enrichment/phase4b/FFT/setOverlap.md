FFT::setOverlap(Double percentageOfOverlap) -> undefined

Thread safety: SAFE
Sets the overlap ratio for chunk-based FFT processing. Clamped to [0.0, 0.99].
An overlap of 0.5 means 50% overlap with step size of fftSize * 0.5 samples.
Higher overlap produces smoother analysis at the cost of more processing iterations.

Dispatch/mechanics:
  overlap = jlimit(0.0, 0.99, input)
  spectrumParameters->oversamplingFactor = nextPowerOfTwo(1.0 / (1.0 - overlap))
  E.g., overlap 0.5 -> factor 2, overlap 0.75 -> factor 4

Pair with:
  setWindowType -- window function and overlap work together for spectral quality
  process -- uses the overlap value to determine chunk stepping
  prepare -- call setOverlap before prepare

Source:
  ScriptingApiObjects.cpp  ScriptFFT::setOverlap()
    -> jlimit(0.0, 0.99, percentageOfOverlap)
    -> spectrumParameters->oversamplingFactor = nextPowerOfTwo(1.0 / (1.0 - overlap))
