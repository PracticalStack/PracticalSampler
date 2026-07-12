Passes a value from HiseScript to a `uniform` variable declared in the GLSL shader. The uniform must be declared in the GLSL source with a matching name and compatible type:

| HiseScript Value | GLSL Type |
|-----------------|-----------|
| Number (double) | float |
| Integer | int |
| Array (2 elements) | vec2 |
| Array (3 elements) | vec3 |
| Array (4 elements) | vec4 |
| Buffer | float[] |

Declare the uniform in GLSL, then call `setUniformData` with the matching name:

```cpp
// GLSL side
uniform float myValue;
uniform vec3 myColour;
uniform float myBuffer[128];
```

```javascript
// HiseScript side
shd.setUniformData("myValue", 0.8);
shd.setUniformData("myColour", [0.0, 1.0, 0.0]);

const var buffer = Buffer.create(128);
shd.setUniformData("myBuffer", buffer);
```

When using a Buffer, the GPU receives a copy of the data at the time of the call - it does not reference the Buffer directly. Call `setUniformData` again whenever the Buffer contents change.

> [!Warning:Built-in uniforms are overwritten each frame] Setting values for `iTime`, `uOffset`, `iResolution`, or `uScale` has no lasting effect because the engine overwrites them on every render frame. Only `iMouse` among the built-in uniforms can be user-controlled via this method.

> [!Warning:Buffer size must match GLSL declaration] The Buffer size determines the GLSL array size. A `Buffer.create(128)` maps to `uniform float myArray[128]`. Mismatched sizes silently drop data or read zeros. Arrays with more than 4 elements must use Buffer - raw Arrays larger than vec4 are silently ignored.
