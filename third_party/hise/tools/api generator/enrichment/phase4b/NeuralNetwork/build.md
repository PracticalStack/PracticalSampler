NeuralNetwork::build(JSON modelJSON) -> undefined

Thread safety: UNSAFE -- acquires ScopedMultiWriteLock, allocates DynamicModel on heap
Builds a neural network from a JSON layer description. The JSON is an array of layer
objects with type, name, inputs, outputs, isActivation fields. Supported layer types:
Linear, Tanh, ReLU, Sigmoid. After building, call loadWeights to set trained parameters.
Required setup:
  const var nn = Engine.createNeuralNetwork("MyNetwork");
Dispatch/mechanics:
  PytorchParser::createModel() -> RTNeural::Model<float> built dynamically from layer list
    -> postBuild() allocates input/output VariantBuffer if multi-input/output
Pair with:
  loadWeights -- must load trained parameters after build
  createModelJSONFromTextFile -- parse Pytorch print(model) output into layer JSON
Anti-patterns:
  - Do NOT call build without calling loadWeights afterwards -- the model has random
    initial weights and will produce meaningless output
  - Calling build replaces the current model entirely -- previously loaded weights are
    lost and loadWeights must be called again
  - [BUG] The scripting wrapper ignores the Result returned by the core build. Unsupported
    layer types fail silently, leaving the model in an undefined state
Source:
  ScriptingApiObjects.cpp:5740  ScriptNeuralNetwork::build()
    -> PytorchParser(modelJSON).createModel()
    -> nn->setModel(newModel) under ScopedMultiWriteLock
    -> postBuild() allocates VariantBuffer for multi-I/O
