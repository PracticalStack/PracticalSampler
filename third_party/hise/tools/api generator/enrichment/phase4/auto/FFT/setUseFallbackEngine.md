Forces the FFT object to use the JUCE fallback FFT engine instead of platform-optimised implementations (such as vDSP on macOS or IPP on Windows). The fallback engine is required for `dumpSpectrum()` to work.

This flag is read during `prepare()` when the FFT engine instance is created.

> [!Warning:Must be set before prepare] Calling `setUseFallbackEngine(true)` after `prepare()` sets the internal flag but does not recreate the existing engine. The fallback only activates on the next `prepare()` call. Always call `setUseFallbackEngine()` before `prepare()`.
