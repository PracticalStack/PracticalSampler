## setUniformData

**Examples:**

```javascript:buffer-uniform-waterfall
// Title: Timer-driven note visualization with Buffer uniforms
// Context: Stream note event data from a FixObjectFactory stack to a GLSL
//          shader every frame. Each stack property is bulk-copied into a
//          Buffer, then uploaded as a shader uniform array.

const var notePrototype = {
    "noteNumber": 0,
    "startTime": -1.0,
    "endTime": -1.0,
    "velocity": 0
};

const var factory = Engine.createFixObjectFactory(notePrototype);
const var noteStack = factory.createStack(128);

// Pre-allocate Buffers that will be sent as GLSL float[] uniforms
const var startBuffer = Buffer.create(128);
const var endBuffer = Buffer.create(128);
const var noteBuffer = Buffer.create(128);

const var shd = Content.createShader("waterfall");

const var pnl = Content.getComponent("ShaderPanel");

pnl.setPaintRoutine(function(g)
{
    g.applyShader(shd, this.getLocalBounds(0));
});

// 30ms timer: update data, upload uniforms, trigger repaint
pnl.setTimerCallback(function()
{
    // Bulk-copy one property from every stack element into a Buffer
    noteStack.copy("startTime", startBuffer);
    noteStack.copy("endTime", endBuffer);
    noteStack.copy("noteNumber", noteBuffer);

    // Upload Buffers as float[] uniforms
    shd.setUniformData("start", startBuffer);
    shd.setUniformData("end", endBuffer);
    shd.setUniformData("keys", noteBuffer);

    // Float scalar uniform
    shd.setUniformData("scrollSpeed", 0.003);

    this.repaint();
});

pnl.startTimer(30);
```
```json:testMetadata:buffer-uniform-waterfall
{
  "testable": false,
  "skipReason": "Requires shader file, FixObjectFactory data, and active OpenGL context"
}
```

```javascript:dsp-cable-to-shader
// Title: Forwarding DSP values and colours to the shader
// Context: Read a value from a GlobalCable (e.g. an envelope follower in a
//          DSP network) and pass it as a float uniform. Convert a HISE
//          colour to vec4 for GLSL consumption.

const var rm = Engine.getGlobalRoutingManager();
const var envelopeCable = rm.getCable("EnvelopeLevel");

const var shd = Content.createShader("visualizer");
const var pnl = Content.getComponent("VisualizerPanel");

reg smoothedValue = 0.0;

pnl.setTimerCallback(function()
{
    // Smooth the DSP value for visual display
    local raw = envelopeCable.getValue();
    smoothedValue = smoothedValue * 0.6 + raw * 0.4;

    // Float uniform
    shd.setUniformData("envelopeGain", smoothedValue);

    // vec4 uniform from a HISE colour (0xAARRGGBB -> [r, g, b, a])
    shd.setUniformData("tintColor", Colours.toVec4(0xFF5FC0D9));

    this.repaint();
});

pnl.startTimer(30);
```
```json:testMetadata:dsp-cable-to-shader
{
  "testable": false,
  "skipReason": "Requires GlobalCable with DSP source, shader file, and active OpenGL context"
}
```

**Pitfalls:**
- When using Buffers as uniforms, the Buffer size determines the GLSL array size. A `Buffer.create(128)` maps to `uniform float myArray[128]` in the shader. Mismatched sizes between HiseScript and GLSL will silently drop data or read zeros.
