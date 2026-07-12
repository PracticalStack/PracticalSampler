## fromBase64

**Signature:** `undefined fromBase64(String b64)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.fromBase64(encodedShader);`

**Description:**
Loads and compiles a shader from a base64-encoded, zstd-compressed GLSL string. This is the counterpart to `toBase64()` for distributing shader code without separate .glsl files. The string is decoded, decompressed, and compiled as raw fragment shader code (the engine auto-prepends built-in uniforms, coordinate macros, and preprocessor definitions).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| b64 | String | no | Base64-encoded zstd-compressed GLSL source code | Must be produced by `toBase64()` |

**Pitfalls:**
- [BUG] If the base64 string is invalid or cannot be decoded, the method silently does nothing -- no shader is compiled and no error is reported.

**Cross References:**
- `$API.ScriptShader.toBase64$`
- `$API.ScriptShader.setFragmentShader$`

## getOpenGLStatistics

**Signature:** `JSON getOpenGLStatistics()`
**Return Type:** `JSON`
**Call Scope:** safe
**Minimal Example:** `var stats = {obj}.getOpenGLStatistics();`

**Description:**
Returns a JSON object containing OpenGL GPU information. The statistics are populated on the OpenGL render thread during the first shader compilation pass. The returned object has the properties: `VersionString` (String, full GL version), `Major` (int, GL major version), `Minor` (int, GL minor version), `Vendor` (String, GPU vendor), `Renderer` (String, GPU model), and `GLSL Version` (String, shader language version). If no OpenGL context is active, the properties contain placeholder values ("Inactive" and zero).

**Parameters:**
(none)

**Pitfalls:**
- Returns undefined if called before the shader has been rendered at least once, since the statistics object is populated during the first GPU compilation pass on the render thread.

**Cross References:**
None.

**Example:**
```javascript:gl-statistics-query
// Title: Querying GPU capabilities
const var pnl = Content.addPanel("ShaderPanel", 0, 0);
const var shd = Content.createShader("");

pnl.setPaintRoutine(function(g)
{
    g.applyShader(shd, [0, 0, this.getWidth(), this.getHeight()]);
});

// After the first render, query GPU info
var stats = shd.getOpenGLStatistics();

if (isDefined(stats))
{
    Console.print("GPU: " + stats.Renderer);
    Console.print("OpenGL: " + stats.Major + "." + stats.Minor);
}
```
```json:testMetadata:gl-statistics-query
{
  "testable": false,
  "skipReason": "Requires active OpenGL context for meaningful statistics"
}
```

## setBlendFunc

**Signature:** `undefined setBlendFunc(Integer enabled, Integer sFactor, Integer dFactor)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setBlendFunc(true, {obj}.GL_SRC_ALPHA, {obj}.GL_ONE);`

**Description:**
Controls OpenGL alpha blending for the shader output. When enabled, the specified source and destination blend factors determine how the shader output is composited with the existing framebuffer content. The blend factors use the GL_* constants available on the shader object. When blending is enabled, the previous GL blend state is saved before rendering and restored afterward. Default blend factors (before any call) are GL_SRC_ALPHA (source) and GL_ONE_MINUS_SRC_ALPHA (destination).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| enabled | Integer | no | Enables or disables custom blending | true/false |
| sFactor | Integer | no | Source blend factor | Must be a GL_* constant from the shader object |
| dFactor | Integer | no | Destination blend factor | Must be a GL_* constant from the shader object |

**Pitfalls:**
None.

**Cross References:**
None.

**Example:**
```javascript:blend-func-additive
// Title: Additive blending for glow effects
const var pnl = Content.addPanel("GlowPanel", 0, 0);
pnl.set("width", 256);
pnl.set("height", 256);

const var shd = Content.createShader("glow");

// Additive blend: output = src * srcAlpha + dst * 1
shd.setBlendFunc(true, shd.GL_SRC_ALPHA, shd.GL_ONE);

pnl.setPaintRoutine(function(g)
{
    g.fillAll(0xFF000000);
    g.applyShader(shd, [0, 0, this.getWidth(), this.getHeight()]);
});
```
```json:testMetadata:blend-func-additive
{
  "testable": false,
  "skipReason": "Requires active OpenGL context for shader rendering"
}
```

## setEnableCachedBuffer

**Signature:** `undefined setEnableCachedBuffer(Integer shouldEnable)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setEnableCachedBuffer(true);`

**Description:**
Enables or disables GPU-to-CPU buffer capture after each shader render. When enabled, every frame's shader output is read back from the GPU via `glReadPixels` into a CPU-side image buffer. This is required for `Content.createScreenshot()` to capture shader output, which would otherwise be unavailable for CPU-side image export. Enabling this adds per-frame GPU readback overhead and should only be used when screenshot capture is needed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldEnable | Integer | no | Enables or disables the cached buffer | true/false |

**Pitfalls:**
None.

**Cross References:**
- `$API.Content.createScreenshot$`

## setFragmentShader

**Signature:** `undefined setFragmentShader(String shaderFile)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setFragmentShader("myEffect");`

**Description:**
Loads a GLSL fragment shader from the Scripts folder and compiles it. The filename should be specified without the .glsl extension. The loaded code is a fragment shader `main()` function -- the engine automatically prepends built-in uniforms, coordinate macros, and any preprocessor definitions. In the backend, if the file does not exist, a default Shadertoy-style template is created. The file is registered with the file watcher for live reloading. GLSL files support `#include "otherFile"` directives for recursive include resolution. Circular includes are detected and throw an error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shaderFile | String | no | Name of the .glsl file in the Scripts folder (without extension) | -- |

**Pitfalls:**
- [BUG] In frontend (exported plugin) builds, if the shader file name does not match an embedded file in the script collection, the shader silently fails to compile with no error message.

**Cross References:**
- `$API.ScriptShader.fromBase64$`
- `$API.ScriptShader.setPreprocessor$`
- `$API.ScriptShader.toBase64$`

## setPreprocessor

**Signature:** `undefined setPreprocessor(String name, NotUndefined value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setPreprocessor("HIGH_QUALITY", 1);`

**Description:**
Adds or updates a `#define` preprocessor directive in the shader source and triggers recompilation. The define is prepended before all built-in uniforms and the user's shader code, so it can be used for conditional compilation (`#ifdef`) or numeric constants. Passing an empty string as the name clears all preprocessor definitions. The value is converted to a string via `toString()`. Every call triggers a full shader recompile.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | Preprocessor define name; empty string clears all defines | -- |
| value | NotUndefined | no | Value for the define (converted to string) | -- |

**Pitfalls:**
- Every call recompiles the shader, even if the value has not changed. Calling this in a tight loop or timer without guarding against unchanged values causes unnecessary GPU recompilation overhead.

**Cross References:**
- `$API.ScriptShader.setFragmentShader$`

**Example:**
```javascript:preprocessor-variants
// Title: Shader variants via preprocessor defines
const var shd = Content.createShader("effect");

// Enable a quality level in the shader code
// (shader can use: #ifdef HIGH_QUALITY ... #endif)
shd.setPreprocessor("HIGH_QUALITY", 1);

// Set a numeric constant
shd.setPreprocessor("NUM_ITERATIONS", 32);

// Clear all preprocessor definitions and recompile
shd.setPreprocessor("", 0);
```
```json:testMetadata:preprocessor-variants
{
  "testable": false,
  "skipReason": "Requires shader file and OpenGL context for compilation"
}
```

## setUniformData

**Signature:** `undefined setUniformData(String id, NotUndefined data)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Minimal Example:** `{obj}.setUniformData("brightness", 0.8);`

**Description:**
Sets a custom uniform variable that is passed to the fragment shader on every render frame. The HiseScript value type determines the GLSL uniform type: a double maps to `float`, an integer maps to `int`, a 2-element array maps to `vec2`, a 3-element array to `vec3`, a 4-element array to `vec4`, and a Buffer maps to a `float[]` array. The uniform must be declared in the GLSL source with a matching name and type.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | Uniform variable name matching the GLSL declaration | -- |
| data | NotUndefined | no | Value to pass to the shader (Number, Integer, Array, or Buffer) | Array must have 2-4 elements for vec2/vec3/vec4 |

**Pitfalls:**
- Setting a value for the built-in uniforms `iTime`, `uOffset`, `iResolution`, or `uScale` has no lasting effect -- the engine overwrites these on every render frame. Only `iMouse` among the built-in uniforms can be user-controlled via this method.
- If the uniform name does not match any declaration in the GLSL source, the value is silently ignored with no error.
- [BUG] Arrays with 1 or more than 4 elements are silently ignored -- only sizes 2, 3, and 4 are mapped to vec2/vec3/vec4.

**Cross References:**
- `$API.ScriptShader.setFragmentShader$`

**Example:**
```javascript:uniform-data-types
// Title: Passing different data types as shader uniforms
const var pnl = Content.addPanel("ShaderPanel", 0, 0);
pnl.set("width", 256);
pnl.set("height", 256);

const var shd = Content.createShader("visualizer");

// Float uniform -> GLSL float
shd.setUniformData("brightness", 0.8);

// Integer uniform -> GLSL int
shd.setUniformData("mode", 2);

// Array uniform -> GLSL vec3
shd.setUniformData("tintColor", [1.0, 0.5, 0.0]);

// Feed mouse position from the panel's mouse callback
pnl.setMouseCallback(function(event)
{
    shd.setUniformData("iMouse", [event.x, event.y]);
    this.repaint();
});

pnl.setPaintRoutine(function(g)
{
    g.applyShader(shd, [0, 0, this.getWidth(), this.getHeight()]);
});
```
```json:testMetadata:uniform-data-types
{
  "testable": false,
  "skipReason": "Requires shader file and OpenGL context for rendering"
}
```

## toBase64

**Signature:** `String toBase64()`
**Return Type:** `String`
**Call Scope:** unsafe
**Minimal Example:** `var encoded = {obj}.toBase64();`

**Description:**
Returns the current shader source code as a base64-encoded, zstd-compressed string. This captures the raw user code (without the auto-prepended header and preprocessor definitions) so it can be stored and later restored via `fromBase64()`. Useful for embedding shader code directly in scripts without requiring separate .glsl files.

**Parameters:**
(none)

**Pitfalls:**
None.

**Cross References:**
- `$API.ScriptShader.fromBase64$`
- `$API.ScriptShader.setFragmentShader$`
