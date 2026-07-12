## copy

**Examples:**

```javascript:copy-columnar-shader-data
// Title: Feeding stack properties to a shader as uniform arrays
// Context: A visualization system copies per-event property columns from
// the stack into Buffers, then passes them as shader uniform data.
// copy() reads all allocated slots (including unused ones with default
// values), which works here because the shader uses the stack size to
// know how many entries are valid.

const var f = Engine.createFixObjectFactory({
    "start": -1.0,
    "end": -1.0,
    "key": 0,
    "velocity": 0
});

const var NUM_SLOTS = 128;
const var stack = f.createStack(NUM_SLOTS);

// Preallocate Buffers matching the stack capacity
const var startBuffer = Buffer.create(NUM_SLOTS);
const var endBuffer = Buffer.create(NUM_SLOTS);
const var keyBuffer = Buffer.create(NUM_SLOTS);

const var shader = Content.createShader("visualizer.glsl");

// In a timer callback: copy columns and update shader
inline function updateShaderData()
{
    stack.copy("start", startBuffer);
    stack.copy("end", endBuffer);
    stack.copy("key", keyBuffer);

    shader.setUniformData("start", startBuffer);
    shader.setUniformData("end", endBuffer);
    shader.setUniformData("key", keyBuffer);

    // Tell the shader how many entries are valid
    shader.setUniform("numActive", stack.size());
}
```
```json:testMetadata:copy-columnar-shader-data
{
  "testable": false,
  "skipReason": "Requires external GLSL shader file (visualizer.glsl) and GPU context"
}
```

The `copy()` method extracts a single property from every allocated slot into a flat Buffer - a columnar data layout ideal for GPU consumption. Unused slots beyond `size()` contain the factory's default values (here `-1.0`), so downstream consumers can either use a count uniform or check for sentinel values.
