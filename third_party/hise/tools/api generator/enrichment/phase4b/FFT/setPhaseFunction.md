FFT::setPhaseFunction(Function newPhaseFunction) -> undefined

Thread safety: UNSAFE -- acquires a write lock, creates a new WeakCallbackHolder (heap allocation), and triggers reinitialise() which reallocates work buffers
Registers a callback that receives phase data for each FFT chunk during process().
Modifications to the phase buffer are used during inverse FFT reconstruction.
For multi-channel input, the first argument is an Array of Buffers.

Callback signature: f(Buffer phases, int offset)

Dispatch/mechanics:
  Acquires ScopedWriteLock -> creates WeakCallbackHolder(2 args)
  USE_BACKEND: RealtimeSafetyInfo::check() validates callback for audio-thread safety
  Calls reinitialise() if prepare() was called previously (reallocates phaseBuffer)

Pair with:
  setMagnitudeFunction -- register magnitude callback for full spectral access
  setEnableInverseFFT -- phase modifications only matter with inverse reconstruction
  process -- invokes the callback per chunk

Anti-patterns:
  - Do NOT use a non-inline function -- must be an inline function with 2 parameters
  - Do NOT set after prepare() if avoidable -- triggers unnecessary buffer reallocation

Source:
  ScriptingApiObjects.cpp  ScriptFFT::setPhaseFunction()
    -> ScopedWriteLock -> new WeakCallbackHolder(p, this, var(), 2)
    -> USE_BACKEND: RealtimeSafetyInfo::check(co, this, "FFT.setPhaseFunction")
    -> reinitialise() -> prepare(lastSpecs)
