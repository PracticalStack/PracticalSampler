NeuralNetwork::loadTensorFlowModel(JSON modelJSON) -> undefined

Thread safety: UNSAFE -- acquires ScopedMultiWriteLock, allocates TensorFlowModel on heap
Loads a TensorFlow model from JSON data. The JSON must contain the full model definition
including weights (RTNeural's TensorFlow JSON format). No separate build or loadWeights
step needed -- topology and weights are combined.
Required setup:
  const var nn = Engine.createNeuralNetwork("MyTF");
Dispatch/mechanics:
  new TensorFlowModel(modelJSON) under ScopedMultiWriteLock
    -> RTNeural::json_parser::parseJson(modelJSON)
    -> postBuild() allocates VariantBuffer if multi-I/O
Anti-patterns:
  - Do NOT call loadWeights after loadTensorFlowModel -- fails with "Tensor Flow models
    will initialise their weights with the model JSON"
  - [BUG] The scripting wrapper ignores the Result returned by the core method. The core
    method has no try-catch around the TensorFlowModel constructor, so malformed JSON
    may cause an unhandled exception
Source:
  ScriptingApiObjects.cpp  ScriptNeuralNetwork::loadTensorFlowModel()
    -> new TensorFlowModel(modelJSON) under ScopedMultiWriteLock
    -> postBuild()
