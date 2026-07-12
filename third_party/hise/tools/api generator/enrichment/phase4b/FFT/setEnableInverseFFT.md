FFT::setEnableInverseFFT(Integer shouldApplyReverseTransformToInput) -> undefined

Thread safety: UNSAFE -- triggers reinitialise() which reallocates all work buffers under a write lock when the state changes
Enables or disables inverse FFT reconstruction. When enabled, process() reconstructs
the time-domain signal from (possibly modified) magnitude and phase data using
overlap-add and returns the result. When disabled, process() returns undefined.

Dispatch/mechanics:
  Sets enableInverse flag, then calls reinitialise() if state changed and prepare() was called previously
  reinitialise() re-runs prepare() with stored specs, reallocating chunkOutput/magBuffer/phaseBuffer

Pair with:
  process -- returns reconstructed audio only when inverse is enabled
  setMagnitudeFunction/setPhaseFunction -- modify spectral data before reconstruction

Source:
  ScriptingApiObjects.cpp  ScriptFFT::setEnableInverseFFT()
    -> sets enableInverse -> reinitialise() -> prepare(lastSpecs.blockSize, lastSpecs.numChannels)
