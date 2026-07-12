NeuralNetwork::reset() -> undefined

Thread safety: UNSAFE -- acquires blocking ScopedReadLock. NAM models run warmup inference (prewarm) during reset.
Resets the internal state of the neural network model. For DynamicModel and TensorFlowModel,
zeros the internal layer states. For NAM models, calls prewarm() which runs warmup inference
with zero input. Automatically called after loadWeights.
Dispatch/mechanics:
  ScopedReadLock -> ModelBase::reset()
    -> DynamicModel/TensorFlowModel: zero internal state
    -> NAMModel: prewarm() runs inference with zero input to initialize wavenet state
Pair with:
  loadWeights -- reset is called automatically after weight loading
  process -- reset clears state between inference runs if needed
Source:
  ScriptingApiObjects.cpp  ScriptNeuralNetwork::reset()
    -> nn->reset() under ScopedReadLock
    -> ModelBase::reset() (virtual dispatch to model implementation)
