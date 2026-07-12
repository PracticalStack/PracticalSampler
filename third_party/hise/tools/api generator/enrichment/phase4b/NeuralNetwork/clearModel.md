NeuralNetwork::clearModel() -> undefined

Thread safety: UNSAFE -- acquires ScopedMultiWriteLock, allocates EmptyModel on heap
Replaces the current model with an empty no-op model. After calling this, process
returns zeros. Useful for releasing model resources before loading a different model.
Dispatch/mechanics:
  nn->setModel(new EmptyModel()) under ScopedMultiWriteLock
Pair with:
  build / loadPytorchModel / loadTensorFlowModel / loadNAMModel -- load a new model after clearing
Source:
  ScriptingApiObjects.cpp  ScriptNeuralNetwork::clearModel()
    -> nn->setModel(EmptyModel) under ScopedMultiWriteLock
