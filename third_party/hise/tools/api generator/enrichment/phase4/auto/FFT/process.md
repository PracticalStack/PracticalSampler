Runs the FFT processing pipeline on the input audio data. Accepts a single Buffer (mono) or an Array of Buffers (multi-channel).

Two independent code paths can be active simultaneously:

1. **Spectrum2D path** - generates a spectrogram image from the input buffer (runs first). The image can be drawn with `Graphics.drawFFTSpectrum()`.
2. **Callback path** - processes data in overlapping chunks. Each chunk is windowed, forward-transformed, and passed to the magnitude and/or phase callbacks.

When inverse FFT is enabled, modified spectral data is reconstructed via overlap-add and returned as a Buffer (mono) or Array of Buffers (multi-channel). When inverse FFT is disabled, the method returns `undefined`.

Throws an error if `prepare()` was not called, or if neither callbacks nor Spectrum2D mode is active.

[See: FFT Processing Pipeline](#diagram-fft-processing-pipeline)
