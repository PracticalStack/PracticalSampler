FFT::getSpectrum2DParameters() -> JSON

Thread safety: UNSAFE -- allocates a new DynamicObject and performs string property operations via saveToJSON
Returns the current Spectrum2D display parameters as a JSON object containing all
configurable spectrogram properties.

Required setup:
  const var fft = Engine.createFFT();

Pair with:
  setSpectrum2DParameters -- configure the parameters this method reads back

Source:
  ScriptingApiObjects.cpp  ScriptFFT::getSpectrum2DParameters()
    -> Spectrum2D::Parameters::saveToJSON() -> new DynamicObject with all parameter properties
