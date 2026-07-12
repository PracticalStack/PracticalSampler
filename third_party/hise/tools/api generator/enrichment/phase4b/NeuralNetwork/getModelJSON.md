NeuralNetwork::getModelJSON() -> JSON

Thread safety: UNSAFE -- acquires ScopedReadLock, constructs JSON objects on heap
Returns the JSON layer description of the currently loaded model. Only works with
DynamicModel (from build) and TensorFlowModel (from loadTensorFlowModel). Returns
an empty object for NAM models, EmptyModel, or compiled models.
Anti-patterns:
  - Returns an empty object silently for NAM models -- only DynamicModel and
    TensorFlowModel support JSON export
Source:
  ScriptingApiObjects.cpp  ScriptNeuralNetwork::getModelJSON()
    -> nn->getModelJSON() under ScopedReadLock
