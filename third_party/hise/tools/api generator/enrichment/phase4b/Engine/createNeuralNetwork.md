Engine::createNeuralNetwork(String id) -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a neural network object wrapping RTNeural for pre-trained model inference.
The id parameter is a unique identifier for this network instance. After creation,
load a trained model before processing audio. Used for amp modeling, effects emulation,
or ML-based audio processing within scriptnode.
Source:
  ScriptingApi.cpp  Engine::createNeuralNetwork()
    -> new ScriptNeuralNetwork(id)
