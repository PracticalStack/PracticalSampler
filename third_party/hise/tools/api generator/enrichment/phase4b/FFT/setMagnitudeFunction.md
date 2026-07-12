FFT::setMagnitudeFunction(Function newMagnitudeFunction, Number convertToDecibels) -> undefined

Thread safety: UNSAFE -- acquires a write lock, creates a new WeakCallbackHolder (heap allocation), and triggers reinitialise() which reallocates work buffers
Registers a callback that receives magnitude data for each FFT chunk during process().
For multi-channel input, the first argument is an Array of Buffers instead of a single Buffer.
In the HISE IDE, the callback is validated for realtime safety.

Callback signature: f(Buffer magnitudes, int offset)

Dispatch/mechanics:
  Acquires ScopedWriteLock -> creates WeakCallbackHolder(2 args)
  USE_BACKEND: RealtimeSafetyInfo::check() validates callback for audio-thread safety
  Calls reinitialise() if prepare() was called previously (reallocates magBuffer)

Pair with:
  setPhaseFunction -- register phase callback for full spectral access
  process -- invokes the callback per chunk
  prepare -- allocates buffers (set callbacks before prepare to avoid double allocation)

Anti-patterns:
  - Do NOT use a non-inline function -- must be an inline function with 2 parameters
  - Do NOT set after prepare() if avoidable -- triggers unnecessary buffer reallocation

Source:
  ScriptingApiObjects.cpp  ScriptFFT::setMagnitudeFunction()
    -> ScopedWriteLock -> new WeakCallbackHolder(p, this, var(), 2)
    -> USE_BACKEND: RealtimeSafetyInfo::check(co, this, "FFT.setMagnitudeFunction")
    -> reinitialise() -> prepare(lastSpecs)
