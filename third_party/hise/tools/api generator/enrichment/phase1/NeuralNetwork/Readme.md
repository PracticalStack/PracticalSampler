# NeuralNetwork -- Class Analysis

## Brief
Machine-learning inference engine for PyTorch, TensorFlow, NAM, and ONNX models in real-time audio.

## Purpose
NeuralNetwork provides a scripting API wrapper around the RTNeural framework and ONNX runtime for running trained neural network models within HISE. It supports multiple model formats (PyTorch, TensorFlow, NAM wavenet, ONNX) through a unified interface, handling model loading, weight initialization, and inference. The class manages thread-safe model swapping via lock-free audio-thread patterns and integrates with scriptnode's global cable routing for bidirectional data flow between DSP networks and script-level inference.

## Details

### Architecture

NeuralNetwork operates as a scripting wrapper (`ScriptNeuralNetwork`) around a shared core `NeuralNetwork` object stored in `MainController::NeuralNetwork::Holder`. Multiple script references using the same ID share the same underlying network instance. The core object is also accessible from scriptnode via the `math.neural` node through the `runtime_target::source_base` interface.

### Two Independent Subsystems

The class contains two completely independent inference engines:

1. **RTNeural subsystem** (`HISE_INCLUDE_RT_NEURAL`) -- Handles PyTorch, TensorFlow, and NAM models. Uses the RTNeural C++ library for real-time inference. Most methods require this flag.
2. **ONNX subsystem** -- Handles ONNX models via a dynamically loaded DLL (`onnx_lib`). Designed for image-based spectral classification rather than sample-level audio processing. Does not require `HISE_INCLUDE_RT_NEURAL`.

### Model Type Hierarchy

All models implement the internal `ModelBase` interface (`process`, `reset`, `clone`, `loadWeights`):

| Model Type | Created By | Topology |
|------------|-----------|----------|
| EmptyModel | Default / `clearModel()` | No-op placeholder |
| DynamicModel | `build()` | User-defined layer JSON (Linear, Tanh, ReLU, Sigmoid) |
| TensorFlowModel | `loadTensorFlowModel()` | Parsed from TF JSON (weights embedded) |
| NAMModel | `loadNAMModel()` | Fixed wavenet topology, always 1-in/1-out |
| CompiledModel | C++ factory registration | Compile-time optimized (production deployment) |

### Model Loading Workflows

See individual method entries for full details: `createModelJSONFromTextFile`, `build`, `loadWeights` (two-step PyTorch), `loadPytorchModel`, `loadTensorFlowModel`, `loadNAMModel` (combined loaders), and `loadOnnxModel` / `processFFTSpectrum` (ONNX path).

### Supported Pytorch Layer Types

- `Linear` (Dense)
- `Tanh` (activation)
- `ReLU` (activation)
- `Sigmoid` (activation)
- `Sequential` (container -- parsed but removed from layer list)

### Input/Output Dispatch

See `process` for full input/output dispatch details. Accepts a single number, Array, or Buffer; returns a float (single output) or Buffer reference (multiple outputs).

### Thread Safety

Model swaps use `ScopedMultiWriteLock` (blocks audio briefly). Processing uses `ScopedTryReadLock` -- if a model swap is in progress, inference is silently skipped rather than blocking. This ensures the audio thread never stalls.

### C++ Code Generation

The `CppBuilder` class can generate optimized C++ model code from a JSON layer description. This enables a development-to-production workflow: prototype with dynamic models in script, then compile optimized static models and register them with the factory for deployment.

## obtainedVia
`Engine.createNeuralNetwork(id)`

## minimalObjectToken
nn

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `nn.process(input)` without calling `build()` or a load method first | Call `nn.build(modelJSON)` or `nn.loadPytorchModel(json)` before `nn.process()` | The default EmptyModel produces no output. A model must be loaded before inference. |
| `nn.loadWeights(weightsJSON)` after `loadTensorFlowModel()` | Use `loadTensorFlowModel()` alone (weights are embedded) | TensorFlow models include weights in the model JSON. Calling `loadWeights()` on a TensorFlowModel returns an error. |
| `nn.processFFTSpectrum(fft, w, h)` without `loadOnnxModel()` | Call `nn.loadOnnxModel(base64Data, numOutputs)` first | The ONNX model must be loaded before spectral inference. Reports a script error otherwise. |

## codeExample
```javascript
// Create a neural network and load a PyTorch model
const var nn = Engine.createNeuralNetwork("myModel");

// Load combined PyTorch model (layers + weights in one JSON)
nn.loadPytorchModel(modelJSON);

// Run inference
var result = nn.process(0.5);
```

## Alternatives
- `FFT` -- Use for standard spectral analysis; NeuralNetwork is for learned audio transformations via neural network inference.
- `LorisManager` -- Use for deterministic partial-tracking resynthesis; NeuralNetwork is for data-driven inference using trained models.

## Related Preprocessors
`HISE_INCLUDE_RT_NEURAL` -- Required for all RTNeural-based methods (PyTorch, TensorFlow, NAM). The ONNX subsystem operates independently.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All methods operate independently with no inter-method timeline dependencies that survive bug fixes. The build-then-loadWeights sequence fails with a runtime error (once the Result-ignoring bug is fixed), and loadOnnxModel/processFFTSpectrum already validate preconditions with reportScriptError. No parse-time diagnostics warranted.
