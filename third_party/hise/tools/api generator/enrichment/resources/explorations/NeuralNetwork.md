# NeuralNetwork -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` (NeuralNetwork entry)
- `enrichment/base/NeuralNetwork.json` (12 API methods)
- No prerequisite classes required

## Source Files
- **Class declaration:** `hi_scripting/scripting/api/ScriptingApiObjects.h:1630-1709` (`ScriptNeuralNetwork`)
- **Implementation:** `hi_scripting/scripting/api/ScriptingApiObjects.cpp:5740-6086`
- **Core infrastructure:** `hi_tools/hi_neural/hi_neural.h` (core `NeuralNetwork` struct, 218 lines)
- **Core implementation:** `hi_tools/hi_neural/hi_neural.cpp` (951 lines -- RTNeural integration, model types, factory)
- **ONNX loader:** `hi_tools/hi_neural/onnx_loader.h` (DLL-based ONNX runtime)
- **Factory method:** `hi_scripting/scripting/api/ScriptingApi.cpp:3495-3498` (`Engine::createNeuralNetwork`)

## Class Declaration

```cpp
class ScriptNeuralNetwork: public ConstScriptingObject
{
public:
    ScriptNeuralNetwork(ProcessorWithScriptingContent* p, const String& name);
    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("NeuralNetwork"); }
    // ... 12 API methods ...
private:
    void postBuild();
    ReferenceCountedObjectPtr<ReferenceCountedObject> outputCableUntyped;
    struct CableInputCallback;
    ScopedPointer<CableInputCallback> cableInput;
    float* getConnectionPtr(bool getInput);  // returns input or output buffer write pointer
    VariantBuffer::Ptr inputBuffer;
    VariantBuffer::Ptr outputBuffer;
    struct Wrapper;
#if HISE_INCLUDE_RT_NEURAL
    NeuralNetwork::Ptr nn;
#endif
    ONNXLoader::Ptr onnx;
    std::vector<float> onnxOutput;
    JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptNeuralNetwork);
};
```

Inherits `ConstScriptingObject` with 0 constants.

## Preprocessor Guards

The class has TWO independent subsystems controlled by different preprocessor flags:

1. **`HISE_INCLUDE_RT_NEURAL`** -- Controls the RTNeural-based inference engine. Most methods (`build`, `process`, `clearModel`, `reset`, `loadWeights`, `loadTensorFlowModel`, `loadPytorchModel`, `loadNAMModel`, `createModelJSONFromTextFile`, `getModelJSON`) require this. Without it, these methods call `reportScriptError("You must enable HISE_INCLUDE_RT_NEURAL")`.

2. **ONNX subsystem** -- `loadOnnxModel` and `processFFTSpectrum` use the `ONNXLoader` which is DLL-based and does NOT require `HISE_INCLUDE_RT_NEURAL`. ONNX loads a separate dynamic library from `tools/onnx_lib` (backend) or the app data directory (frontend).

These are completely independent code paths. A build can have RTNeural without ONNX and vice versa.

## Constructor and Method Registration

```cpp
ScriptNeuralNetwork(ProcessorWithScriptingContent* p, const String& name):
    ConstScriptingObject(p, 0)  // 0 constants
{
    ADD_API_METHOD_1(process);
    ADD_API_METHOD_0(clearModel);
    ADD_API_METHOD_1(build);
    ADD_API_METHOD_0(reset);
    ADD_API_METHOD_1(loadWeights);
    ADD_API_METHOD_1(createModelJSONFromTextFile);
    ADD_API_METHOD_1(loadTensorFlowModel);
    ADD_API_METHOD_1(loadPytorchModel);
    ADD_API_METHOD_1(loadNAMModel);
    ADD_API_METHOD_0(getModelJSON);
    ADD_API_METHOD_2(loadOnnxModel);
    ADD_API_METHOD_3(processFFTSpectrum);

#if HISE_INCLUDE_RT_NEURAL
    nn = p->getMainController_()->getNeuralNetworks().getOrCreate(Identifier(name));
#endif
}
```

All methods use `ADD_API_METHOD_N` (no typed variants). The `name` parameter becomes a unique ID in the `NeuralNetwork::Holder` registry on the `MainController`. Multiple script references to the same ID share the same underlying `NeuralNetwork` instance.

## Factory / ObtainedVia

Created via `Engine.createNeuralNetwork(id)`:

```cpp
ScriptingObjects::ScriptNeuralNetwork* ScriptingApi::Engine::createNeuralNetwork(String id)
{
    return new ScriptingObjects::ScriptNeuralNetwork(getScriptProcessor(), id);
}
```

The `id` is used to look up or create a `NeuralNetwork` instance in the `MainController`'s `NeuralNetwork::Holder`:

```cpp
NeuralNetwork::Ptr NeuralNetwork::Holder::getOrCreate(const Identifier& id)
{
    for(auto nn: networks)
        if(id == nn->getId())
            return nn;
    auto nn = new NeuralNetwork(id, getFactory());
    networks.add(nn);
    return Ptr(nn);
}
```

This means calling `Engine.createNeuralNetwork("myNN")` twice returns two `ScriptNeuralNetwork` wrappers pointing to the SAME underlying `NeuralNetwork` object.

## Core NeuralNetwork Infrastructure (hi_tools/hi_neural/)

### NeuralNetwork struct

```cpp
struct NeuralNetwork: public ReferenceCountedObject,
                      public runtime_target::source_base
```

Key aspects:
- Reference counted, shared across scripting wrappers and scriptnode nodes
- Implements `runtime_target::source_base` for scriptnode integration (the `math.neural` node connects to it)
- Contains `ProcessingContext` struct controlling processing mode
- Holds an `OwnedArray<ModelBase>` for multi-channel/polyphonic clones
- Thread-safe via `SimpleReadWriteLock` -- model swaps use `ScopedMultiWriteLock`, processing uses `ScopedTryReadLock` (non-blocking on audio thread)

### ProcessingContext

```cpp
struct ProcessingContext
{
    int numSamplesPerCall = 1;   // chunk size for batch processing
    int fixChannelSize = 0;      // 0 = clone per channel, >0 = multichannel frame
    bool shouldCloneChannels() const { return fixChannelSize == 0; }
    int getSignalInputSize() const;  // audio signal inputs (remaining are parameters)
};
```

This is primarily used by the scriptnode `math.neural` node, not directly exposed via the scripting API.

### ModelBase Interface

```cpp
struct ModelBase
{
    virtual void reset() = 0;
    virtual void process(const float* input, float* output) = 0;
    virtual int getNumInputs() const = 0;
    virtual int getNumOutputs() const = 0;
    virtual ModelBase* clone() = 0;
    virtual Result loadWeights(const String& jsonData) = 0;
};
```

### Model Implementations

| Model Class | Created By | Description |
|-------------|-----------|-------------|
| `EmptyModel` | Default factory / `clearModel()` | No-op placeholder, all methods are empty |
| `DynamicModel` | `build()` | Parses layer JSON via `PytorchParser`, creates `RTNeural::Model<float>` dynamically |
| `TensorFlowModel` | `loadTensorFlowModel()` | Loads from TF JSON via `RTNeural::json_parser::parseJson` |
| `NAMModel` | `loadNAMModel()` | Wavenet-based Neural Amp Modeler (fixed topology: 1 in, 1 out) |
| `CompiledModel<T>` | Factory registration | Template for statically compiled models (C++ code generation target) |

### Factory and Compiled Model Registration

```cpp
struct Factory
{
    Factory();  // sets defaultFunction = []() { return new EmptyModel(); }
    template <typename T> void registerModel();  // register compiled models
    ModelBase* create(const Identifier& id);     // lookup by id, fallback to EmptyModel
};
```

The factory allows pre-compiled models to be registered. When `getOrCreate(id)` is called, if a registered model matches the ID, it's used instead of an EmptyModel. This is the deployment path: train in Python -> export to C++ via `CppBuilder` -> compile into plugin -> register with factory.

### CppBuilder

Generates C++ source code for a compiled RTNeural model from the JSON layer description:

```cpp
struct CppBuilder
{
    CppBuilder(const Identifier& id_, const var& modelJson);
    String createCppModel() const;  // requires HISE_INCLUDE_SNEX
};
```

Output example (from the source):
```cpp
namespace pimpl
{
using l1_t = RTNeural::DenseT<float, 1, 16>;
using t1_t = RTNeural::ReLuActivationT<float, 16>;
// ...
using MyFunk_t = RTNeural::ModelT<float, 1, 1, l1_t, t1_t, l2_t, t2_t, l3_t>;
}

struct MyFunk: public CompiledModel<pimpl::MyFunk_t>
{
    RTN_MODEL_ID(MyFunk);
    Result loadWeights(const String& jsonData) final { ... }
};
```

### PytorchParser

Parses the text output of Python's `print(model)` into a structured layer list:

```cpp
class PytorchParser
{
    struct LayerInfo { Identifier type; String name; int inputs, outputs; bool isActivationFunction; };
    static var createJSONModel(const String& modelLayout);
    ModelPtr createModel();
    Result loadWeights(const ModelPtr& model, const nlohmann::json& modelJson);
};
```

Supported layer types (from `PytorchIds` namespace):
- **Containers:** `Sequential`
- **Layers:** `Linear` (Dense)
- **Activations:** `Tanh`, `ReLU`, `Sigmoid`

### NAMModel (Neural Amp Modeler)

Fixed-topology wavenet model for guitar amp simulation:

```cpp
struct NAMModel: public NeuralNetwork::ModelBase
{
    using Dilations = wavenet::Dilations<1, 2, 4, 8, 16, 32, 64, 128, 256, 512>;
    // Always 1 input, 1 output
    // Uses math_approx::tanh<3> for fast activation
    wavenet::Wavenet_Model<float, 1,
        wavenet::Layer_Array<float, 1, 1, 8, 16, 3, Dilations, false, NAMMathsProvider>,
        wavenet::Layer_Array<float, 16, 1, 1, 8, 3, Dilations, true, NAMMathsProvider>> obj;
};
```

NAM models load weights directly (no separate build step) and are always mono (1 in, 1 out).

## ONNX Subsystem

Completely separate from RTNeural. Uses a dynamically loaded DLL (`onnx_lib`):

```cpp
class ONNXLoader: public ReferenceCountedObject
{
    // Function pointers loaded from DLL
    typedef void* (*loadModel_f)(const void*, size_t);
    typedef bool (*run_f)(void*, const void*, size_t, int, bool);
    typedef void (*getOutput_f)(void*, float*);
    // ...
    bool run(const Image& img, std::vector<float>& outputValues, bool isGreyscale);
    Result loadModel(const MemoryBlock& mb);
};
```

Key details:
- Model data is passed as base64-encoded binary (`loadOnnxModel`)
- Input is an `Image` (from FFT spectrum), not raw floats
- Output is a flat float array
- DLL path: `HISE_PATH/tools/onnx_lib` (backend) or app data dir (frontend)
- Uses `SharedResourcePointer<SharedData>` for DLL singleton

The ONNX path is designed for spectral classification (image-based inference), not real-time sample processing.

## Global Cable Integration

`connectToGlobalCables` bridges the neural network with scriptnode's global routing system:

```cpp
struct CableInputCallback: public scriptnode::routing::GlobalRoutingManager::CableTargetBase
{
    void sendValue(double v) override
    {
        // When cable receives a value, run inference
        if(numInputs == 1)
            parent->process(v);
        else
        {
            auto input = parent->getConnectionPtr(false);
            input[0] = (float)v;
            parent->process(var(parent->inputBuffer.get()));
        }
    }
};
```

- Input cable: triggers inference when a value arrives
- Output cable: sends the first output value after processing
- This enables scriptnode-to-script neural network communication

## postBuild() -- Buffer Allocation

Called after any model loading operation (`build`, `loadTensorFlowModel`, `loadPytorchModel`, `loadNAMModel`):

```cpp
void postBuild()
{
    auto numInputs = nn->getNumInputs();
    auto numOutputs = nn->getNumOutputs();
    if(numInputs > 1) inputBuffer = new VariantBuffer(numInputs);
    if(numOutputs > 1) outputBuffer = new VariantBuffer(numOutputs);
}
```

Only allocates buffers when the network has multiple inputs/outputs. Single-input/output models bypass buffer allocation entirely.

## Threading Model

The core `NeuralNetwork` uses `SimpleReadWriteLock`:
- **Model swaps** (`build`, `loadTensorFlowModel`, `loadNAMModel`, `clearModel`, `setNumNetworks`): `ScopedMultiWriteLock` -- blocks audio
- **Processing** (`process`): `ScopedTryReadLock` -- non-blocking, silently skips if model is being swapped
- **Queries** (`getNumInputs`, `getNumOutputs`): `ScopedReadLock` -- blocking read
- **Weight loading**: `ScopedMultiWriteLock`
- **Reset and warmup**: `ScopedReadLock`

The scripting wrapper (`ScriptNeuralNetwork`) does NOT add its own locking -- it relies on the core lock.

## process() Input/Output Dispatch

The `process` method handles multiple input/output configurations:

| Inputs | Outputs | Input Type | Behavior |
|--------|---------|-----------|----------|
| 1 | 1 | Number | Single float in, single float out |
| N | 1 | Array | Array copied to inputBuffer, single float out |
| N | 1 | Buffer | Buffer pointer used directly, single float out |
| 1 | M | Number | Single float in, outputBuffer returned |
| N | M | Array | Array copied to inputBuffer, outputBuffer returned |
| N | M | Buffer | Buffer pointer used directly, outputBuffer returned |

When an output cable is connected, the first output value is always sent to it regardless of the number of outputs.

Return type: single float `var` when 1 output, `VariantBuffer` reference when multiple outputs.

## Supported Model Formats Summary

| Format | Load Method | Separate Weight Loading | Build Required | Notes |
|--------|------------|------------------------|----------------|-------|
| Pytorch (manual) | `build()` + `loadWeights()` | Yes | Yes | Two-step: build from layer JSON, then load weights |
| Pytorch (combined) | `loadPytorchModel()` | No (embedded) | No | JSON with "layers" and "weights" keys |
| TensorFlow | `loadTensorFlowModel()` | No (embedded) | No | Full TF JSON with weights included |
| NAM | `loadNAMModel()` | No (embedded) | No | Wavenet topology, always 1-in/1-out |
| ONNX | `loadOnnxModel()` | N/A | N/A | Separate subsystem, image-based inference |
| Compiled C++ | Factory registration | At init | At compile time | Production deployment path |

## runtime_target Integration

`NeuralNetwork` inherits `runtime_target::source_base`, which enables the scriptnode `math.neural` node to connect to it at runtime. The `getRuntimeHash()` returns the hash of the ID string, and `getType()` returns `RuntimeTarget::NeuralNetwork`. This is the mechanism by which scriptnode nodes discover and bind to named neural network instances.

## Wrapper Registration (No Typed Methods)

All 12 methods use untyped `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N`:

```cpp
API_METHOD_WRAPPER_1(ScriptNeuralNetwork, process);
API_VOID_METHOD_WRAPPER_0(ScriptNeuralNetwork, clearModel);
API_VOID_METHOD_WRAPPER_1(ScriptNeuralNetwork, build);
API_VOID_METHOD_WRAPPER_0(ScriptNeuralNetwork, reset);
API_VOID_METHOD_WRAPPER_1(ScriptNeuralNetwork, loadWeights);
API_METHOD_WRAPPER_0(ScriptNeuralNetwork, getModelJSON);
API_VOID_METHOD_WRAPPER_1(ScriptNeuralNetwork, loadTensorFlowModel);
API_VOID_METHOD_WRAPPER_1(ScriptNeuralNetwork, loadPytorchModel);
API_VOID_METHOD_WRAPPER_1(ScriptNeuralNetwork, loadNAMModel);
API_METHOD_WRAPPER_1(ScriptNeuralNetwork, createModelJSONFromTextFile);
API_METHOD_WRAPPER_2(ScriptNeuralNetwork, loadOnnxModel);
API_METHOD_WRAPPER_3(ScriptNeuralNetwork, processFFTSpectrum);
```

No `ADD_TYPED_API_METHOD_N` calls -- all parameters are dynamically typed.

## Holder on MainController

```cpp
// MainController.h
#if HISE_INCLUDE_RT_NEURAL
    NeuralNetwork::Holder& getNeuralNetworks() { return neuralNetworks; }
    // ...
    NeuralNetwork::Holder neuralNetworks;
#endif
```

The Holder maintains a `ReferenceCountedArray<NeuralNetwork>` and a `Factory` pointer. Neural networks persist for the lifetime of the MainController and are shared across all script processors.
