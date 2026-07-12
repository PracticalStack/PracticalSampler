## getDisplayBuffer

**Examples:**

```javascript:compressor-gain-reduction-meter
// Title: Compressor gain reduction meter
// Context: Dynamics processors expose multiple display buffers -
// typically gain reduction at index 0 and peak level at index 1.
// Store the source when you need more than one buffer from the
// same processor.

const var compSource = Synth.getDisplayBufferSource("MyCompressor");
const var gainRedBuffer = compSource.getDisplayBuffer(0);
const var peakBuffer = compSource.getDisplayBuffer(1);

const var CompPanel = Content.getComponent("CompPanel");

CompPanel.setTimerCallback(function()
{
    // createPath builds a vector path from the ring buffer contents
    this.data.peakPath = peakBuffer.createPath(
        this.getLocalBounds(0), [0.0, 1.0, 0, -1], 0.0);

    this.repaint();
});

CompPanel.setPaintRoutine(function(g)
{
    g.fillAll(0xFF222222);
    g.setColour(0x33FFFFFF);

    if (isDefined(this.data.peakPath))
        g.fillPath(this.data.peakPath, this.getLocalBounds(0));
});

CompPanel.startTimer(30);
```
```json:testMetadata:compressor-gain-reduction-meter
{
  "testable": false,
  "skipReason": "Requires a dynamics processor with multiple display buffers and visual timer/paint rendering"
}
```

```javascript:batch-fft-setup
// Title: Batch FFT setup for multiple EQ processors
// Context: In a channel strip architecture, each channel has its own
// parametric EQ. Collect all display buffers in an array during onInit,
// then configure identical FFT properties for each.

const var NUM_CHANNELS = 4;
const var eqBuffers = [];

for (i = 0; i < NUM_CHANNELS; i++)
{
    local eqId = "ChannelEQ" + (i + 1);
    local db = Synth.getDisplayBufferSource(eqId).getDisplayBuffer(0);
    eqBuffers.push(db);
}

// Configure FFT properties for all buffers
const var fftProperties = {
    "BufferLength": 2048,
    "WindowType": "Flat Top",
    "DecibelRange": [-80.0, -6.0],
    "UsePeakDecay": false,
    "UseDecibelScale": true,
    "YGamma": 0.6,
    "Decay": 0.65,
    "UseLogarithmicFreqAxis": true
};

for (db in eqBuffers)
    db.setRingBufferProperties(fftProperties);
```
```json:testMetadata:batch-fft-setup
{
  "testable": false,
  "skipReason": "Requires multiple parametric EQ processors in the module tree"
}
```

**Pitfalls:**
- When a processor exposes multiple display buffers (common for dynamics processors), the buffer indices are determined by the processor's C++ implementation. There is no API to query how many buffers exist or what each index represents - you must know the processor's buffer layout.
