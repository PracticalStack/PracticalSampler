FFT::setEnableSpectrum2D(Integer shouldBeEnabled) -> undefined

Thread safety: SAFE
Enables or disables 2D spectrogram image generation during process(). The generated
image can be drawn using Graphics.drawFFTSpectrum(). Configure appearance with
setSpectrum2DParameters().

Pair with:
  setSpectrum2DParameters -- configure spectrogram appearance
  process -- generates the spectrum image when this is enabled

Source:
  ScriptingApiObjects.cpp  ScriptFFT::setEnableSpectrum2D()
    -> sets enableSpectrum flag
