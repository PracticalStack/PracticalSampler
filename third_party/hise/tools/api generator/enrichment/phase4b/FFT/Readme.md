FFT (object)
Obtain via: Engine.createFFT()

Windowed FFT processor for spectral analysis, magnitude/phase callbacks,
inverse resynthesis, and 2D spectrogram generation. Operates on Buffer objects
with configurable overlap-add processing and up to 16 channels.

Constants:
  WindowType:
    Rectangle = 0      Rectangular (no) window function
    Triangle = 1       Triangular window function
    Hamming = 2        Hamming window function
    Hann = 3           Hann window function
    BlackmanHarris = 4 Blackman-Harris window function
    Kaiser = 5         Kaiser window function
    FlatTop = 6        Flat-top window function

Common mistakes:
  - Calling process() without prepare() first -- throws "You must call prepare
    before process".
  - Calling dumpSpectrum() without setUseFallbackEngine(true) before prepare()
    -- throws "You must use the fallback engine if you want to dump FFT images".
  - Calling setUseFallbackEngine(true) after prepare() -- flag is set but the
    engine is not recreated until the next prepare() or reinitialise().

Example:
  const var fft = Engine.createFFT();

  fft.setWindowType(fft.Hann);
  fft.setOverlap(0.5);
  fft.setMagnitudeFunction(function(magnitudes, offset)
  {
      // magnitudes is a Buffer with frequency bin amplitudes
      // offset is the current position in the source buffer
  }, false);

  fft.prepare(1024, 1);

Methods (12):
  getSpectrum2DParameters   prepare
  process                   setEnableInverseFFT
  setEnableSpectrum2D       setMagnitudeFunction
  setOverlap                setPhaseFunction
  setSpectrum2DParameters   setUseFallbackEngine
  setUseSpectrumList        setWindowType
