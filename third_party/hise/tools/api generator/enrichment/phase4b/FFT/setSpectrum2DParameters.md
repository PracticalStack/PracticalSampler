FFT::setSpectrum2DParameters(JSON jsonData) -> undefined

Thread safety: UNSAFE -- calls loadFromJSON which performs string property lookups and value assignments
Configures the Spectrum2D spectrogram renderer from a JSON object. Only included
properties are updated; omitted properties retain current values.

Properties:
  FFTSize: int            Log2 of FFT size (7=128 to 13=8192)
  DynamicRange: int       Minimum dB for display (default: 110)
  Oversampling: int       Oversampling factor (default: 4)
  ColourScheme: int       0=blackWhite, 1=rainbow, 2=violetToOrange, 3=hiseColours, 4=preColours
  GainFactor: int         Gain where 1000 = 0.0 dB (default: 1000)
  ResamplingQuality: str  "Low", "Mid", or "High"
  Gamma: int              Gamma correction 0-150 (default: 60)
  Standardize: int        Standardize output (boolean, default: false)
  FrequencyGamma: int     Frequency axis gamma 100-200 (default: 100)
  WindowType: int         Window type 0-6 (same as FFT window constants)

Pair with:
  getSpectrum2DParameters -- read back current configuration
  setEnableSpectrum2D -- must enable Spectrum2D for these parameters to take effect

Source:
  ScriptingApiObjects.cpp  ScriptFFT::setSpectrum2DParameters()
    -> Spectrum2D::Parameters::loadFromJSON(jsonData)
