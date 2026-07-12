Enables or disables inverse FFT reconstruction. When enabled, `process()` reconstructs the time-domain signal from the (possibly modified) magnitude and phase data using overlap-add, and returns the result as a Buffer or Array of Buffers. When disabled, `process()` calls the callbacks but does not reconstruct or return audio data.

Toggling this after `prepare()` triggers automatic buffer reallocation only if the state actually changes.
