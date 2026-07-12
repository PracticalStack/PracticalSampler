Exports the display buffer state as a base64-encoded string. The returned string can be passed to `fromBase64()` to restore the state later.

> [!Warning:Only supported by stateful buffer types] Returns an empty string without error if the buffer type does not support state serialisation. Most types (FFT, oscilloscope, goniometer) do not implement export - only envelope types produce meaningful output.
