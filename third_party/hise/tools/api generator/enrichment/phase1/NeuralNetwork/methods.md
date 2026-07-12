# NeuralNetwork -- Method Documentation

## build

**Signature:** `undefined build(JSON modelJSON)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires ScopedMultiWriteLock, allocates DynamicModel on heap.
**Minimal Example:** `{obj}.build(layerData);`

**Description:**
Builds a neural network from a JSON layer description. The JSON is an array of layer objects produced by `createModelJSONFromTextFile` or constructed manually. After building, call `loadWeights` to set the trained parameters. Supported layer types: Linear, Tanh, ReLU, Sigmoid. Requires `HISE_INCLUDE_RT_NEURAL`. Internally allocates input/output buffers matching the model's dimensions.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| modelJSON | JSON | no | Array of layer objects describing the network topology | Must be a valid layer array |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| type | String | Layer type: "Linear", "Tanh", "ReLU", or "Sigmoid" |
| name | String | Layer name used as prefix for weight loading |
| inputs | Integer | Number of input connections for this layer |
| outputs | Integer | Number of output connections for this layer |
| isActivation | Integer | 1 if activation function, 0 if learnable layer |

**Pitfalls:**
- Calling `build` replaces the current model entirely. Previously loaded weights are lost and `loadWeights` must be called again.
- [BUG] The scripting wrapper ignores the Result returned by the core `build`. If the JSON contains unsupported layer types, the error is silently discarded and the model remains in an undefined state.

**Cross References:**
- `$API.NeuralNetwork.loadWeights$`
- `$API.NeuralNetwork.createModelJSONFromTextFile$`
- `$API.NeuralNetwork.loadPytorchModel$`

**Example:**
```javascript:build-simple-network
// Title: Build a simple neural network from layer JSON
const var nn = Engine.createNeuralNetwork("MyNetwork");

var layers = [
    {"type": "Linear", "name": "l1", "inputs": 1, "outputs": 16, "isActivation": false},
    {"type": "ReLU", "name": "relu1", "inputs": 16, "outputs": 16, "isActivation": true},
    {"type": "Linear", "name": "l2", "inputs": 16, "outputs": 1, "isActivation": false}
];

nn.build(layers);
```

```json:testMetadata:build-simple-network
{
  "testable": false,
  "skipReason": "Requires HISE_INCLUDE_RT_NEURAL build flag"
}
```

## clearModel

**Signature:** `undefined clearModel()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires ScopedMultiWriteLock, allocates EmptyModel on heap.
**Minimal Example:** `{obj}.clearModel();`

**Description:**
Replaces the current model with an empty no-op model. After calling this, `process` returns zeros. Useful for releasing model resources before loading a different model. Requires `HISE_INCLUDE_RT_NEURAL`.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.NeuralNetwork.build$`
- `$API.NeuralNetwork.loadPytorchModel$`
- `$API.NeuralNetwork.loadTensorFlowModel$`
- `$API.NeuralNetwork.loadNAMModel$`
- `$API.NeuralNetwork.loadOnnxModel$`

## connectToGlobalCables

**Signature:** `undefined connectToGlobalCables(String inputId, String outputId)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Accesses GlobalRoutingManager, creates CableTargetBase objects on heap.
**Minimal Example:** `{obj}.connectToGlobalCables("NNInput", "NNOutput");`

**Description:**
Connects the neural network to global routing cables for automatic input/output. When a value arrives on the input cable, it triggers inference via `process`. The first output value is sent to the output cable after processing. Pass an empty string for either parameter to skip that connection.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| inputId | String | no | Global cable ID for input values | Empty string to skip |
| outputId | String | no | Global cable ID for output values | Empty string to skip |

**Pitfalls:**
- [BUG] This method is implemented in C++ but not registered via `ADD_API_METHOD_2` in the constructor and has no `Wrapper` entry. It cannot be called from HISEScript.

**Cross References:**
- `$API.NeuralNetwork.process$`

## createModelJSONFromTextFile

**Signature:** `JSON createModelJSONFromTextFile(ScriptObject fileObject)`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** File I/O (reads file content from disk).
**Minimal Example:** `var layers = {obj}.createModelJSONFromTextFile(modelFile);`

**Description:**
Parses a Pytorch model text file (output of Python's `print(model)`) and returns a JSON array describing the network layers. The returned JSON can be passed directly to `build`. Supports Sequential containers with Linear, Tanh, ReLU, and Sigmoid layers. Requires `HISE_INCLUDE_RT_NEURAL`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fileObject | ScriptObject | no | A File object pointing to the .txt model file | Must be a valid ScriptFile |

**Pitfalls:**
- [BUG] Returns an empty object silently if the argument is not a valid ScriptFile. No error message is shown.

**Cross References:**
- `$API.NeuralNetwork.build$`
- `$API.NeuralNetwork.loadWeights$`

## getModelJSON

**Signature:** `JSON getModelJSON()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Acquires ScopedReadLock, constructs JSON objects on heap.
**Minimal Example:** `var json = {obj}.getModelJSON();`

**Description:**
Returns the JSON layer description of the currently loaded model. Only works with DynamicModel (from `build`) and TensorFlowModel (from `loadTensorFlowModel`). Returns an empty object for NAM models, EmptyModel, or compiled models. Requires `HISE_INCLUDE_RT_NEURAL`.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Returns an empty object silently for NAM models. Only DynamicModel and TensorFlowModel support JSON export.

**Cross References:**
- `$API.NeuralNetwork.build$`
- `$API.NeuralNetwork.loadTensorFlowModel$`

## loadNAMModel

**Signature:** `undefined loadNAMModel(JSON modelJSON)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires ScopedMultiWriteLock, allocates NAMModel on heap.
**Minimal Example:** `{obj}.loadNAMModel(namData);`

**Description:**
Loads a Neural Amp Modeler (NAM) wavenet model from JSON data. NAM models use a fixed wavenet topology and are always mono (1 input, 1 output), designed for guitar amp simulation. The JSON contains the wavenet weights directly -- no separate `build` or `loadWeights` step is needed. Requires `HISE_INCLUDE_RT_NEURAL`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| modelJSON | JSON | no | NAM wavenet model data with weights | Must be valid NAM JSON |

**Pitfalls:**
- [BUG] The scripting wrapper ignores the Result returned by the core method. If weight loading fails (e.g., malformed JSON), the error is silently discarded.

**Cross References:**
- `$API.NeuralNetwork.loadPytorchModel$`
- `$API.NeuralNetwork.loadTensorFlowModel$`
- `$API.NeuralNetwork.loadOnnxModel$`

## loadOnnxModel

**Signature:** `Integer loadOnnxModel(String base64Data, Integer numOutputs)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** DLL loading, heap allocations, base64 decoding.
**Minimal Example:** `{obj}.loadOnnxModel(encodedData, 10);`

**Description:**
Loads an ONNX model from base64-encoded binary data. This is a separate subsystem from RTNeural and does NOT require `HISE_INCLUDE_RT_NEURAL`. ONNX models are used for spectral classification via `processFFTSpectrum`, not real-time sample processing. The ONNX runtime DLL is loaded from `HISE_PATH/tools/onnx_lib` (backend) or the app data directory (frontend). Returns 1 on success, throws a script error on failure.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| base64Data | String | no | Base64-encoded ONNX model binary | Must be valid base64 |
| numOutputs | Integer | no | Number of output values the model produces | Must be positive |

**Cross References:**
- `$API.NeuralNetwork.processFFTSpectrum$`
- `$API.NeuralNetwork.loadPytorchModel$`
- `$API.NeuralNetwork.loadTensorFlowModel$`
- `$API.NeuralNetwork.loadNAMModel$`

## loadPytorchModel

**Signature:** `undefined loadPytorchModel(JSON modelJSON)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires ScopedMultiWriteLock, allocates DynamicModel on heap, loads weights.
**Minimal Example:** `{obj}.loadPytorchModel(fullModel);`

**Description:**
Loads a complete Pytorch model with both layer topology and trained weights in a single call. The JSON must have two keys: `"layers"` containing the text output of Python's `print(model)`, and `"weights"` containing the trained parameters exported via PyTorch's `state_dict()`. Internally calls `build` followed by `loadWeights`. Requires `HISE_INCLUDE_RT_NEURAL`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| modelJSON | JSON | no | Combined model with "layers" and "weights" keys | Must contain both keys |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| layers | String | Text output of Python's `print(model)` describing the network topology |
| weights | JSON | Trained model parameters from PyTorch's `state_dict()` |

**Pitfalls:**
- [BUG] The scripting wrapper ignores the Result returned by the core method. If layer parsing or weight loading fails, the error is silently discarded.

**Cross References:**
- `$API.NeuralNetwork.build$`
- `$API.NeuralNetwork.loadWeights$`
- `$API.NeuralNetwork.createModelJSONFromTextFile$`
- `$API.NeuralNetwork.loadTensorFlowModel$`
- `$API.NeuralNetwork.loadNAMModel$`
- `$API.NeuralNetwork.loadOnnxModel$`

**Example:**
```javascript:load-pytorch-combined
// Title: Load a Pytorch model with layers and weights
const var nn = Engine.createNeuralNetwork("MyNetwork");

// fullModel is a JSON object with "layers" (model.txt content)
// and "weights" (state_dict JSON)
var fullModel = {
    "layers": "(l1): Linear(in_features=1, out_features=16)\n(relu1): ReLU()\n(l2): Linear(in_features=16, out_features=1)",
    "weights": {}
};

nn.loadPytorchModel(fullModel);
```

```json:testMetadata:load-pytorch-combined
{
  "testable": false,
  "skipReason": "Requires HISE_INCLUDE_RT_NEURAL build flag"
}
```

## loadTensorFlowModel

**Signature:** `undefined loadTensorFlowModel(JSON modelJSON)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires ScopedMultiWriteLock, allocates TensorFlowModel on heap.
**Minimal Example:** `{obj}.loadTensorFlowModel(tfData);`

**Description:**
Loads a TensorFlow model from JSON data. The JSON must contain the full model definition including weights (RTNeural's TensorFlow JSON format). Unlike the Pytorch two-step workflow, no separate `build` or `loadWeights` step is needed. Requires `HISE_INCLUDE_RT_NEURAL`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| modelJSON | JSON | no | Full TensorFlow model JSON with embedded weights | Must be valid RTNeural TF format |

**Pitfalls:**
- Calling `loadWeights` after loading a TensorFlow model fails with "Tensor Flow models will initialise their weights with the model JSON".
- [BUG] The scripting wrapper ignores the Result returned by the core method. Additionally, the core method has no try-catch around the TensorFlowModel constructor (unlike `build` and `loadNAMModel`), so malformed JSON may cause an unhandled exception.

**Cross References:**
- `$API.NeuralNetwork.loadPytorchModel$`
- `$API.NeuralNetwork.loadNAMModel$`
- `$API.NeuralNetwork.loadOnnxModel$`
- `$API.NeuralNetwork.loadWeights$`

## loadWeights

**Signature:** `undefined loadWeights(JSON weightData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires ScopedMultiWriteLock, JSON serialization to string.
**Minimal Example:** `{obj}.loadWeights(weights);`

**Description:**
Loads trained weights into the current model. Must be called after `build` for Pytorch-style two-step workflows. The weight data is a JSON object matching the format exported by PyTorch's `state_dict()` with the `EncodeTensor` helper class. After loading, the model is automatically reset. Requires `HISE_INCLUDE_RT_NEURAL`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| weightData | JSON | no | Trained model parameters as JSON | Must match current model topology |

**Pitfalls:**
- [BUG] The scripting wrapper ignores the Result returned by the core `loadWeights`. Calling this on an EmptyModel (before `build`) silently fails -- the core returns "network is not initialised" but the error never reaches the user.
- Calling `loadWeights` on a TensorFlow model fails with "Tensor Flow models will initialise their weights with the model JSON", but this error is also silently discarded by the wrapper.

**Cross References:**
- `$API.NeuralNetwork.build$`
- `$API.NeuralNetwork.createModelJSONFromTextFile$`

## process

**Signature:** `var process(var input)`
**Return Type:** `var`
**Call Scope:** warning
**Call Scope Note:** Uses ScopedTryReadLock (non-blocking, skips if write lock held). No allocations on the processing path. Inference computation is O(model_size) per call.
**Minimal Example:** `var result = {obj}.process(0.5);`

**Description:**
Runs inference on the loaded model. Accepts a single float, an array of floats, or a Buffer as input. Returns a single float when the model has one output, or a Buffer reference when it has multiple outputs. If an output cable is connected, the first output value is automatically sent to it. Silently returns zero if a model swap is in progress (write lock held). Requires `HISE_INCLUDE_RT_NEURAL`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| input | NotUndefined | no | Input value(s) for inference | Number, Array, or Buffer matching model input size |

**Pitfalls:**
- [BUG] The input size validation for Array and Buffer inputs uses `isPositiveAndBelow(expectedSize, inputSize)` which requires `inputSize > expectedSize` (strict greater-than). Passing an array or buffer with exactly the model's input count causes the check to fail silently -- no processing occurs and 0.0 is returned. Arrays must have at least one extra element beyond the model's input count.
- Returns 0.0 silently when a write lock is held during model swap. No error or indication that processing was skipped.

**Cross References:**
- `$API.NeuralNetwork.build$`
- `$API.NeuralNetwork.reset$`
- `$API.NeuralNetwork.processFFTSpectrum$`

**Example:**
```javascript:process-inference
// Title: Run inference with different input types
const var nn = Engine.createNeuralNetwork("MyNetwork");

// Single input, single output model
var result = nn.process(0.5);

// Multi-input model: pass array (must have MORE elements than model inputs due to size check)
var multiResult = nn.process([0.1, 0.2, 0.3, 0.0]);
```

```json:testMetadata:process-inference
{
  "testable": false,
  "skipReason": "Requires HISE_INCLUDE_RT_NEURAL build flag and loaded model"
}
```

## processFFTSpectrum

**Signature:** `Array processFFTSpectrum(ScriptObject fftObject, Integer numFreqPixels, Integer numTimePixels)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Image rescaling, DLL inference call, Array construction on heap.
**Minimal Example:** `var classes = {obj}.processFFTSpectrum(fft, 128, 64);`

**Description:**
Runs ONNX inference on an FFT spectrum for spectral classification. Takes an FFT object and rescales its spectrum to the specified dimensions as an image, then passes it through the loaded ONNX model. Returns an array of float output values (e.g., classification probabilities). Requires `loadOnnxModel` to be called first. Does NOT require `HISE_INCLUDE_RT_NEURAL` -- uses the separate ONNX subsystem.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| fftObject | ScriptObject | no | An FFT object with spectrum data | Must be a valid ScriptFFT |
| numFreqPixels | Integer | no | Height of the rescaled spectrum image | Must be positive |
| numTimePixels | Integer | no | Width of the rescaled spectrum image | Must be positive |

**Cross References:**
- `$API.NeuralNetwork.loadOnnxModel$`
- `$API.NeuralNetwork.process$`

## reset

**Signature:** `undefined reset()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires blocking ScopedReadLock. NAM models run warmup inference (prewarm) during reset.
**Minimal Example:** `{obj}.reset();`

**Description:**
Resets the internal state of the neural network model. For DynamicModel and TensorFlowModel, this zeros the internal layer states. For NAM models, this calls `prewarm()` which runs warmup inference with zero input. Automatically called after `loadWeights`. Requires `HISE_INCLUDE_RT_NEURAL`.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.NeuralNetwork.loadWeights$`
- `$API.NeuralNetwork.process$`
