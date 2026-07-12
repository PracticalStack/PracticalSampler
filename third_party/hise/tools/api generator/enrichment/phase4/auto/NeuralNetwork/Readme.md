<!-- Diagram triage:
  - No diagram specifications exist for this class or its methods.
-->

# NeuralNetwork

NeuralNetwork runs trained neural network models for real-time audio inference. It supports four model formats through a unified interface:

1. **PyTorch** - custom layer topologies with separate or combined weight loading.
2. **TensorFlow** - RTNeural JSON format with embedded weights.
3. **NAM** (Neural Amp Modeler) - fixed wavenet topology for guitar amp modelling (mono only).
4. **ONNX** - spectral classification via a separate runtime (not sample-level audio).

The typical workflow is to create a network in script, load a pre-trained model, and either call `process` for offline inference or connect the network to a `math.neural` scriptnode node for audio-rate processing. Multiple script references using the same ID share the same underlying network instance.

```js
const var nn = Engine.createNeuralNetwork("myModel");
nn.loadPytorchModel(modelJSON);
```

The PyTorch path offers two loading workflows: a single-call `loadPytorchModel` for combined JSON files (layers + weights), or a two-step `build` + `loadWeights` path when topology and weights are managed separately. TensorFlow and NAM models each load in a single call with weights embedded.

> The RTNeural subsystem (PyTorch, TensorFlow, NAM) requires the `HISE_INCLUDE_RT_NEURAL` build flag. The ONNX subsystem is independent and does not require this flag. Model swaps briefly block the audio thread; if a swap is in progress during processing, inference is silently skipped rather than stalling. For audio-rate per-sample processing, use the `math.neural` scriptnode node rather than calling `process` in a script callback - the scriptnode node runs on the audio thread with proper polyphonic voice management.

## Common Mistakes

- **Load a model before calling process**
  **Wrong:** `nn.process(input)` without calling `build()` or a load method first
  **Right:** Call `nn.build(modelJSON)` or `nn.loadPytorchModel(json)` before `nn.process()`
  *The default empty model produces no output. A model must be loaded before inference returns meaningful values.*

- **Do not call loadWeights after loadTensorFlowModel**
  **Wrong:** `nn.loadWeights(weightsJSON)` after `loadTensorFlowModel()`
  **Right:** Use `loadTensorFlowModel()` alone - weights are embedded in the model JSON
  *TensorFlow models include weights in the model JSON. Calling `loadWeights()` on a TensorFlow model returns an error.*

- **Load ONNX model before spectral inference**
  **Wrong:** `nn.processFFTSpectrum(fft, w, h)` without `loadOnnxModel()`
  **Right:** Call `nn.loadOnnxModel(base64Data, numOutputs)` before spectral inference
  *The ONNX model must be loaded before calling `processFFTSpectrum`. Reports a script error otherwise.*

- **Use scriptnode for audio-rate processing**
  **Wrong:** Calling `nn.process(x)` per-sample inside `onTimer` or a script callback
  **Right:** Create the network in script, use the `math.neural` scriptnode node for audio-rate processing
  *Script callbacks cannot sustain per-sample processing at audio rate. The scriptnode node runs on the audio thread with proper buffer handling and voice management.*

- **Use File.loadAsObject for model JSON**
  **Wrong:** Loading model JSON with `Engine.loadFromJSON()` or manual string parsing
  **Right:** Use `File.loadAsObject()` to parse the exported JSON file, then pass the result directly to `loadPytorchModel`
  *The exported model JSON from the Python exporter is a standard JSON object. `File.loadAsObject()` handles parsing directly.*
