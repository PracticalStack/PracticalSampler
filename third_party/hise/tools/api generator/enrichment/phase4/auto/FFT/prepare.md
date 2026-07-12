Allocates all internal buffers required for FFT processing. Must be called before `process()`. The FFT size must be a power of two (256, 512, 1024, 2048, etc.) and the channel count is clamped to [1, 16].

Work buffers are allocated conditionally based on the current configuration: magnitude buffers only when a magnitude callback or inverse FFT is enabled, phase buffers only when a phase callback or inverse FFT is enabled, and output buffers only when inverse FFT is enabled. Changing callbacks or enabling inverse FFT after calling `prepare()` triggers automatic reallocation, so the setup order is flexible.

[See: FFT Processing Pipeline](#diagram-fft-processing-pipeline)
