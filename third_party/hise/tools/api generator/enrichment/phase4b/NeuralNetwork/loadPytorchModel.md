NeuralNetwork::loadPytorchModel(JSON modelJSON) -> undefined

Thread safety: UNSAFE -- acquires ScopedMultiWriteLock, allocates DynamicModel on heap, loads weights
Loads a complete Pytorch model with both layer topology and trained weights in a single
call. The JSON must have two keys: "layers" (text output of Python's print(model)) and
"weights" (trained parameters from PyTorch's state_dict()). Internally calls build
followed by loadWeights.
Required setup:
  const var nn = Engine.createNeuralNetwork("MyNetwork");
Dispatch/mechanics:
  modelJSON["layers"] -> PytorchParser::createModel() -> DynamicModel
    -> modelJSON["weights"] -> PytorchParser::loadWeights()
    -> nn->setModel(newModel) under ScopedMultiWriteLock
    -> postBuild() allocates VariantBuffer if multi-I/O
Anti-patterns:
  - [BUG] The scripting wrapper ignores the Result returned by the core method. If
    layer parsing or weight loading fails, the error is silently discarded
  - The "layers" value must be the exact string output of Python's print(model),
    including newlines and indentation. The parser relies on this format.
Source:
  ScriptingApiObjects.cpp  ScriptNeuralNetwork::loadPytorchModel()
    -> PytorchParser(layers).createModel()
    -> PytorchParser::loadWeights(model, weightsJSON)
    -> nn->setModel() + postBuild()
