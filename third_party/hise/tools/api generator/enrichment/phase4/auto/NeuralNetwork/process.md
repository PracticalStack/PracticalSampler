Runs inference on the loaded model. Accepts a single number, an Array, or a Buffer as input. Returns a single float when the model has one output, or a Buffer reference when it has multiple outputs. If a global output cable is connected via `connectToGlobalCables`, the first output value is automatically sent to it.

> [!Warning:Silent zero during model swap] Returns 0.0 without any error when a model swap is in progress. There is no indication that processing was skipped.

> [!Warning:Use math.neural for audio-rate processing] For per-sample processing at audio rate, use the `math.neural` scriptnode node instead of calling `process` in a script loop. Script callbacks cannot sustain sample-rate processing; the scriptnode node runs on the audio thread with proper buffer handling.
