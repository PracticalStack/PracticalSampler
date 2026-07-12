## loadTensorFlowModel

**Examples:**

```javascript:load-tf-from-file
// Title: Load a TensorFlow model from a JSON file
// Context: TensorFlow models use the RTNeural JSON format where
// topology and weights are combined in one file. No separate
// build or loadWeights step is needed.

const var nn = Engine.createNeuralNetwork("TFDistortion");

// Load the TensorFlow model JSON (e.g., exported from RTNeural's Python tools)
const var scriptsFolder = FileSystem.getFolder(FileSystem.UserPresets).getParentDirectory();
const var modelFile = scriptsFolder.getChildFile("Scripts/Python/model_tf.json");
const var tfJSON = modelFile.loadAsObject();

// Single call loads both topology and weights
nn.loadTensorFlowModel(tfJSON);

// Run inference on a test buffer
const var buf = Buffer.create(512);

for (i = 0; i < buf.length; i++)
    buf[i] = 40.0 * Math.sin(i / buf.length * Math.PI * 2.0);

for (s in buf)
    s = nn.process(s);
```

```json:testMetadata:load-tf-from-file
{
  "testable": false,
  "skipReason": "Requires HISE_INCLUDE_RT_NEURAL build flag and external model file"
}
```
