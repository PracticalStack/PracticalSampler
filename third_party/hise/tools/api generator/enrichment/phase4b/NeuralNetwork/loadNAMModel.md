NeuralNetwork::loadNAMModel(JSON modelJSON) -> undefined

Thread safety: UNSAFE -- acquires ScopedMultiWriteLock, allocates NAMModel on heap
Loads a Neural Amp Modeler (NAM) wavenet model from JSON data. NAM models use a fixed
wavenet topology and are always mono (1 input, 1 output), designed for guitar amp
simulation. The JSON contains weights directly -- no separate build or loadWeights step.
Required setup:
  const var nn = Engine.createNeuralNetwork("MyNAM");
Dispatch/mechanics:
  new NAMModel(modelJSON) under ScopedMultiWriteLock
    -> fixed wavenet topology with dilations 1..512
    -> uses math_approx::tanh<3> for fast activation
    -> postBuild() (no buffer allocation needed -- always 1-in/1-out)
Anti-patterns:
  - Do NOT call loadWeights after loadNAMModel -- NAM models load weights from the
    model JSON directly. There is no separate weight loading step.
  - [BUG] The scripting wrapper ignores the Result returned by the core method. If
    weight loading fails (e.g., malformed JSON), the error is silently discarded
Source:
  ScriptingApiObjects.cpp  ScriptNeuralNetwork::loadNAMModel()
    -> new NAMModel(modelJSON) under ScopedMultiWriteLock
    -> postBuild()
