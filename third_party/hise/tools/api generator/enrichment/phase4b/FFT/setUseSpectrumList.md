FFT::setUseSpectrumList(Integer numRows) -> undefined

Thread safety: UNSAFE -- allocates a new SpectrumList with the specified number of image slots
Creates an internal image list with the specified number of slots for batch spectrum
collection. When active, dumpSpectrum() can store spectrum images by index into this
list for later batch export.

Pair with:
  setUseFallbackEngine -- fallback engine required for dumpSpectrum operations

Source:
  ScriptingApiObjects.cpp  ScriptFFT::setUseSpectrumList()
    -> new SpectrumList(numRows) with std::vector<Image>(numRows)
