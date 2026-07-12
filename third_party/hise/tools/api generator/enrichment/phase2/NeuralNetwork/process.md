## process

**Examples:**

```javascript:offline-buffer-waveshaping
// Title: Offline buffer waveshaping with a trained network
// Context: Apply a learned transfer function to a buffer of samples
// at init time. Useful for previewing model behavior or generating
// wavetables from neural networks.

const var nn = Engine.createNeuralNetwork("Waveshaper");

// modelJSON is a pre-trained PyTorch model loaded from file or embedded as JSON
nn.loadPytorchModel(modelJSON);

// Create a sine wave buffer
const var buf = Buffer.create(512);

for (i = 0; i < buf.length; i++)
    buf[i] = 0.5 * Math.sin(i / buf.length * Math.PI * 2.0);

// Apply the network sample-by-sample
// The trained model approximates tanh(15 * x), acting as a soft clipper
for (s in buf)
    s = nn.process(s);

// buf now contains the waveshaped output
```

```json:testMetadata:offline-buffer-waveshaping
{
  "testable": false,
  "skipReason": "Requires HISE_INCLUDE_RT_NEURAL build flag and a trained model JSON"
}
```

**Pitfalls:**
- For audio-rate per-sample processing, use the `math.neural` scriptnode node instead of calling `process` in a script loop. Script callbacks cannot sustain sample-rate processing; scriptnode runs on the audio thread with proper buffer handling.
