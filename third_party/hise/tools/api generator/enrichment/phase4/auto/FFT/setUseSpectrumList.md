Creates an internal image list with the specified number of slots for batch spectrum collection. When active, `dumpSpectrum()` can store spectrum images by index (passing an integer instead of a File object) into this list for later batch export.

Requires the fallback FFT engine - call `setUseFallbackEngine(true)` before `prepare()`.
