Graphics::drawFFTSpectrum(ScriptObject fftObject, Array area) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, copies a spectrum Image from the FFT object
Draws the 2D spectrum image of an FFT object into the specified area. The FFT object
must be created via Engine.createFFT(). Does not require a layer.

Anti-patterns:
  - If fftObject is not a valid FFT object, the error message incorrectly says
    "not a SVG object" (copy-paste bug from drawSVG)

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawFFTSpectrum()
    -> FFT::getSpectrum(false) retrieves current spectrum image (non-blocking)
    -> Spectrum2D::draw() renders into area
