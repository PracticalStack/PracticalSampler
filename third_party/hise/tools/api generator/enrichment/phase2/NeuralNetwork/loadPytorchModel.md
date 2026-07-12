## loadPytorchModel

**Examples:**

```javascript:load-pytorch-from-file
// Title: Load a PyTorch model from a JSON file
// Context: The standard workflow for loading a pre-trained model exported
// from Python using the EncodeTensor exporter (layers + weights combined).

const var nn = Engine.createNeuralNetwork("MyWaveshaper");

// Load the exported model JSON from the project's Scripts folder
const var scriptsFolder = FileSystem.getFolder(FileSystem.UserPresets).getParentDirectory();
const var modelFile = scriptsFolder.getChildFile("Scripts/Python/model.json");
const var modelJSON = modelFile.loadAsObject();

// Load both topology and weights in one call
nn.loadPytorchModel(modelJSON);

// Test inference: apply the learned transfer function to a buffer
const var buf = Buffer.create(512);

for (i = 0; i < buf.length; i++)
    buf[i] = 0.5 * Math.sin(i / buf.length * Math.PI * 2.0);

for (s in buf)
    s = nn.process(s);
```

```json:testMetadata:load-pytorch-from-file
{
  "testable": false,
  "skipReason": "Requires HISE_INCLUDE_RT_NEURAL build flag and external model file"
}
```

```javascript:inline-model-scriptnode
// Title: Inline model for scriptnode integration
// Context: When the model is small, embed the JSON directly in the script.
// The math.neural scriptnode node references this network by its ID
// for audio-rate polyphonic processing.

const var nn = Engine.createNeuralNetwork("SineApprox");

// Small model with inline weights (exported from Python)
const var model = {
    "layers": "SineNet(\n  (network): Sequential(\n    (0): Linear(in_features=1, out_features=8, bias=True)\n    (1): Tanh()\n    (2): Linear(in_features=8, out_features=4, bias=True)\n    (3): Tanh()\n    (4): Linear(in_features=4, out_features=1, bias=True)\n  )\n)",
    "weights": {
        "network.0.weight": [[1.05], [1.42], [0.95], [1.12], [-2.00], [1.49], [-1.32], [-1.48]],
        "network.0.bias": [-0.45, -1.28, 2.0, -1.04, 0.29, 0.48, 0.32, 0.41],
        "network.2.weight": [[-1.79, -0.38, -0.39, 0.16, 0.55, -1.12, 0.68, 1.33],
                             [0.34, 1.87, -0.22, 2.57, 0.38, -0.18, 0.04, -0.09],
                             [0.31, 0.85, -0.60, 0.97, -1.35, 0.12, -1.17, -0.96],
                             [-0.51, -0.74, 0.31, -0.96, 1.90, -0.15, 1.68, 0.90]],
        "network.2.bias": [-0.70, 0.32, -0.62, 0.18],
        "network.4.weight": [[-0.92, 1.11, -0.90, 0.39]],
        "network.4.bias": [0.07]
    }
};

nn.loadPytorchModel(model);

// The math.neural node in the DSP network uses "SineApprox" as its Model property
// to reference this network for audio-rate processing
```

```json:testMetadata:inline-model-scriptnode
{
  "testable": false,
  "skipReason": "Requires HISE_INCLUDE_RT_NEURAL build flag"
}
```

**Pitfalls:**
- The `"layers"` value must be the exact string output of Python's `print(model)`, including newlines and indentation. The parser relies on this format to identify layer types and dimensions.
