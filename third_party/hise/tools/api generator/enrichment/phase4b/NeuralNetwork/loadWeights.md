NeuralNetwork::loadWeights(JSON weightData) -> undefined

Thread safety: UNSAFE -- acquires ScopedMultiWriteLock, JSON serialization to string
Loads trained weights into the current model. Must be called after build for Pytorch-style
two-step workflows. The weight data is a JSON object matching PyTorch's state_dict()
format. After loading, the model is automatically reset.
Required setup:
  const var nn = Engine.createNeuralNetwork("MyNetwork");
  nn.build(layerJSON);
Dispatch/mechanics:
  weightData -> JSON::toString() -> nn->loadWeights(jsonString)
    -> PytorchParser::loadWeights(model, nlohmann::json)
    -> nn->reset() after successful load
Pair with:
  build -- must build the model topology before loading weights
  createModelJSONFromTextFile -- parse model text to get topology for build
Anti-patterns:
  - [BUG] The scripting wrapper ignores the Result returned by the core loadWeights.
    Calling this on an EmptyModel (before build) silently fails -- the core returns
    "network is not initialised" but the error never reaches the user
  - Calling loadWeights on a TensorFlow model fails with "Tensor Flow models will
    initialise their weights with the model JSON" (also silently discarded)
Source:
  ScriptingApiObjects.cpp  ScriptNeuralNetwork::loadWeights()
    -> nn->loadWeights(jsonString) under ScopedMultiWriteLock
    -> nn->reset()
