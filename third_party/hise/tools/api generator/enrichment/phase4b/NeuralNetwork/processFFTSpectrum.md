NeuralNetwork::processFFTSpectrum(ScriptObject fftObject, Integer numFreqPixels, Integer numTimePixels) -> Array

Thread safety: UNSAFE -- image rescaling, DLL inference call, Array construction on heap
Runs ONNX inference on an FFT spectrum for spectral classification. Takes an FFT object
and rescales its spectrum to the specified dimensions as an image, then passes it through
the loaded ONNX model. Returns an array of float output values (e.g., classification
probabilities). Does NOT require HISE_INCLUDE_RT_NEURAL -- uses the separate ONNX subsystem.
Required setup:
  const var nn = Engine.createNeuralNetwork("Classifier");
  nn.loadOnnxModel(base64Data, numOutputs);
Dispatch/mechanics:
  ScriptFFT -> Image rescaling to (numTimePixels x numFreqPixels)
    -> ONNXLoader::run(image, onnxOutput, isGreyscale)
    -> copy onnxOutput to HISEScript Array
Pair with:
  loadOnnxModel -- must load the ONNX model before calling processFFTSpectrum
Source:
  ScriptingApiObjects.cpp  ScriptNeuralNetwork::processFFTSpectrum()
    -> onnx->run(rescaledImage, onnxOutput)
    -> Array construction from onnxOutput vector
