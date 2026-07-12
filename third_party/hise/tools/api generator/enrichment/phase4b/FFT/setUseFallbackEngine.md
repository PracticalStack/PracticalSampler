FFT::setUseFallbackEngine(Integer shouldUseFallback) -> undefined

Thread safety: SAFE
Forces the FFT object to use the JUCE fallback FFT engine instead of
platform-optimized implementations (vDSP on macOS, IPP on Windows). Required
for dumpSpectrum() to work. The flag is read during prepare() when the FFT
engine instance is created.

Pair with:
  prepare -- reads the fallback flag when creating the FFT engine

Anti-patterns:
  - Do NOT call after prepare() and expect immediate effect -- the flag is set but the
    existing engine is not recreated. Call setUseFallbackEngine() before prepare(), or
    trigger reallocation (e.g., toggle inverse FFT) after changing the flag.

Source:
  ScriptingApiObjects.h  ScriptFFT::setUseFallbackEngine()
    -> useFallback = shouldUseFallback (inline setter)
    -> flag read in prepare() -> new juce::dsp::FFT(order, useFallback)
