Sets the window function applied to each data chunk before the forward FFT transform. Use the window type constants defined on the FFT instance (`fft.Rectangle`, `fft.Triangle`, `fft.Hamming`, `fft.Hann`, `fft.BlackmanHarris`, `fft.Kaiser`, `fft.FlatTop`).

The window type also affects Spectrum2D spectrogram generation. Changing the window type after `prepare()` triggers automatic buffer reallocation.
