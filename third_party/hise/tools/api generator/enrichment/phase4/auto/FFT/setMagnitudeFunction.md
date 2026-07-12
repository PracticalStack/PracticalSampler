Registers a callback that receives magnitude (amplitude) data for each FFT chunk during `process()`. The callback signature is `function(magnitudes, offset)` where `magnitudes` is a Buffer of frequency bin amplitudes and `offset` is the current sample position in the source data. For multi-channel input, `magnitudes` is an Array of Buffers.

When `convertToDecibels` is true, magnitude values are converted to decibels before the callback is invoked. Modifications to the magnitude buffer in the callback are used during inverse FFT reconstruction.

Setting a magnitude function after `prepare()` triggers automatic buffer reallocation.
