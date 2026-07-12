NeuralNetwork::loadOnnxModel(String base64Data, Integer numOutputs) -> Integer

Thread safety: UNSAFE -- DLL loading, heap allocations, base64 decoding
Loads an ONNX model from base64-encoded binary data. Separate subsystem from RTNeural --
does NOT require HISE_INCLUDE_RT_NEURAL. Used for spectral classification via
processFFTSpectrum, not real-time sample processing. ONNX runtime DLL loaded from
HISE_PATH/tools/onnx_lib (backend) or app data directory (frontend). Returns 1 on
success, throws script error on failure.
Required setup:
  const var nn = Engine.createNeuralNetwork("Classifier");
Dispatch/mechanics:
  base64 decode -> MemoryBlock -> ONNXLoader::loadModel(mb)
    -> loads DLL via SharedResourcePointer<SharedData>
    -> allocates onnxOutput vector of size numOutputs
Pair with:
  processFFTSpectrum -- run inference on FFT spectrum after loading
Source:
  ScriptingApiObjects.cpp  ScriptNeuralNetwork::loadOnnxModel()
    -> onnx->loadModel(MemoryBlock from base64)
    -> onnxOutput.resize(numOutputs)
