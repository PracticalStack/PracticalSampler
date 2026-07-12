Registers a callback that receives phase data for each FFT chunk during `process()`. The callback signature is `function(phases, offset)` where `phases` is a Buffer of phase angles per frequency bin and `offset` is the current sample position in the source data. For multi-channel input, `phases` is an Array of Buffers.

Modifications to the phase buffer in the callback are used during inverse FFT reconstruction - for example, randomising phase values produces a spectral diffusion effect.

Setting a phase function after `prepare()` triggers automatic buffer reallocation.
