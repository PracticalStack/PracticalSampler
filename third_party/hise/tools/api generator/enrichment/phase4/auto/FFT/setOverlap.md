Sets the overlap ratio for chunk-based FFT processing. The value is clamped to [0.0, 0.99]. An overlap of 0.5 means each chunk overlaps 50% with the previous one, giving a step size of `fftSize * 0.5` samples. Higher overlap produces smoother spectral analysis at the cost of more processing iterations.

The Spectrum2D oversampling factor is derived automatically as the next power of two of `1 / (1 - overlap)` - for example, overlap 0.5 gives factor 2, overlap 0.75 gives factor 4.
