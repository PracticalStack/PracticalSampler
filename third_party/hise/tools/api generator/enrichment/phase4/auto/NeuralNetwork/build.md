Builds a neural network from a JSON array of layer objects. Each layer object specifies a `type` (Linear, Tanh, ReLU, or Sigmoid), `name`, `inputs`, `outputs`, and `isActivation` flag. After building, call `loadWeights` to set the trained parameters - the model is not ready for inference until weights are loaded.

> [!Warning:Replaces the current model entirely] Calling `build` discards any previously loaded model and its weights. If you need to reload weights after rebuilding, call `loadWeights` again.
