# NeuralNetwork -- Project Context

## Project Context

### Real-World Use Cases
- **Neural waveshaper / nonlinear function approximation**: A plugin trains a small neural network in Python to approximate a nonlinear transfer function (tanh distortion, soft clipping, tube saturation), then loads the trained model at init and runs per-sample inference as a waveshaper effect. The NeuralNetwork class replaces a lookup table or math expression with a learned function.
- **Neural oscillator via scriptnode**: A synthesiser loads a trained model that approximates a waveform shape (e.g., sine wave) and connects it to the `math.neural` scriptnode node. A phasor feeds 0-1 ramp values through the network, producing the learned waveform at audio rate in a polyphonic context. The script creates and initializes the network; scriptnode handles the per-sample processing.

### Complexity Tiers
1. **Basic inference** (most common): `Engine.createNeuralNetwork`, `loadPytorchModel` or `loadTensorFlowModel`, `process`. Load a pre-trained model and run inference from script.
2. **Scriptnode integration**: Same loading workflow, but the `math.neural` node in a DSP network references the script-created network by ID for audio-rate processing with automatic polyphonic channel cloning.
3. **Two-step PyTorch workflow**: `createModelJSONFromTextFile` + `build` + `loadWeights` for cases where model topology and weights are stored separately.
4. **Production deployment**: Export the model to compiled C++ via `CppBuilder`, register with the factory, and use statically-optimized inference with zero runtime parsing overhead.

### Practical Defaults
- Use `loadPytorchModel` with a combined JSON (layers + weights) as the default workflow. The two-step `build` + `loadWeights` path is only needed when topology and weights are managed separately.
- Use `loadTensorFlowModel` for TensorFlow models. Weights are embedded in the model JSON, so no separate weight loading step is needed.
- For audio-rate processing, prefer the `math.neural` scriptnode node over calling `process` per-sample in a script callback. Script creates and loads the model; scriptnode handles the DSP loop.
- NAM models (`loadNAMModel`) are fixed-topology wavenets (1-in, 1-out) designed for guitar amp modelling. Use them only for that purpose.

### Integration Patterns
- `Engine.createNeuralNetwork(id)` + `math.neural` node with matching `Model` property -- Script initializes the network and loads weights at startup; the scriptnode node discovers it by ID via `runtime_target` and runs inference at audio rate.
- `File.loadAsObject()` -> `NeuralNetwork.loadPytorchModel(json)` -- Load exported model JSON from a file on disk using the FileSystem API, then pass the parsed JSON directly to the loader.
- `Buffer.create(n)` + `for(s in b) s = nn.process(s)` -- Offline buffer inference pattern for applying a trained network to a pre-filled sample buffer (e.g., waveshaping a waveform at init time).

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `nn.process(x)` per-sample inside `onTimer` or a script callback for audio-rate use | Create the network in script, use the `math.neural` scriptnode node for audio-rate processing | Script callbacks run on the message thread and cannot sustain per-sample processing at audio rate. The scriptnode node runs on the audio thread with proper voice management. |
| Loading model JSON with `Engine.loadFromJSON()` or manual string parsing | Use `File.loadAsObject()` to parse the exported JSON file, then pass the result directly to `loadPytorchModel` | The exported model JSON (from the Python exporter) is a standard JSON object. `loadAsObject()` handles parsing; no manual conversion needed. |
