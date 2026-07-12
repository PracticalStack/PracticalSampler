<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# ScriptShader

ScriptShader wraps a GLSL fragment shader for GPU-accelerated rendering within a ScriptPanel's paint routine. It lets you run shader programs on the GPU to create complex animations, procedural textures, and data-driven visualisations that would be too expensive on the CPU.

The rendering workflow has three steps:

1. Create a shader object with `Content.createShader("myShader")` (loads `myShader.glsl` from the Scripts folder)
2. In a ScriptPanel's paint routine, call `g.applyShader(shader, area)` to fill a rectangle with the shader output
3. For animations, start a timer on the panel and call `this.repaint()` each tick - the built-in `iTime` uniform advances automatically

```javascript
const var shd = Content.createShader("myEffect");

panel.setPaintRoutine(function(g)
{
    g.applyShader(shd, this.getLocalBounds(0));
});

panel.startTimer(30);
panel.setTimerCallback(function()
{
    this.repaint();
});
```

There are two limitations to shader support in HISE:

- **No texture input** - shaders cannot sample external images or textures (e.g. Shadertoy's `iChannel` inputs are not available)
- **Fragment shaders only** - vertex shaders are not supported; the shader operates on a 2D rectangle

### Built-in Uniforms

The engine automatically prepends these uniforms and macros to every shader:

| Name | GLSL Type | Description |
|------|-----------|-------------|
| `iTime` | float | Elapsed time in seconds (auto-updated each frame) |
| `iResolution` | vec3 | Viewport size in pixels (auto-updated each frame) |
| `fragCoord` | vec2 | Pixel coordinates with Y-flip and scale correction (macro) |
| `fragColor` | vec4 | Output colour alias for `gl_FragColor` (macro) |
| `pixelAlpha` | float | Opacity from the host OpenGL context - **must** be multiplied into the output colour |
| `iMouse` | vec2 | Mouse position - declared but **not** auto-populated; feed it manually via `setUniformData` |
| `uScale` | float | Display scale factor (auto-updated) |
| `uOffset` | vec2 | Viewport offset (auto-updated) |

### Porting Shaders from Shadertoy

[Shadertoy](https://www.shadertoy.com) hosts a large gallery of GLSL shaders that can be adapted for HISE with two modifications:

1. **Rename the main function** - change `void mainImage(out vec4 fragColor, in vec2 fragCoord)` to `void main()` with no parameters
2. **Multiply output by `pixelAlpha`** - change `fragColor = vec4(col, 1.0)` to `fragColor = pixelAlpha * vec4(col, 1.0)`

Before porting, check that the Shadertoy shader has no images in the `iChannel` boxes. If any channel has a texture assigned, the shader relies on texture input and will not work in HISE.

**Shadertoy original:**

```cpp
void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord / iResolution.xy;
    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));
    fragColor = vec4(col, 1.0);
}
```

**HISE version:**

```cpp
void main()
{
    vec2 uv = fragCoord / iResolution.xy;
    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));
    fragColor = pixelAlpha * vec4(col, 1.0);
}
```

### GLSL Constant Declarations

You can define constants above the `main()` function using `#define` or `const`:

```cpp
#define PI 3.14159265359

const float myFloat = 2.35468;
const vec2 myVector = vec2(0.6546, 0.9512);
```

Constant arrays are **not** supported in this GLSL version. This will not compile:

```cpp
// Does not work
const vec2 myArray[2] = vec2[2](vec2(0.4657, 0.2149), vec2(0.5536, 0.1345));
```

As a workaround, declare arrays as local variables inside `main()`:

```cpp
void main()
{
    vec2 myArray[2];
    myArray[0] = vec2(0.4657, 0.2149);
    myArray[1] = vec2(0.5536, 0.1345);
    // ...
}
```

### Blend Mode Constants

`setBlendFunc` uses the following OpenGL blend factor constants, accessed as properties on the shader object:

| Constant | Value | Description |
|----------|-------|-------------|
| `ScriptShader.GL_ZERO` | 0 | Factor: (0, 0, 0, 0) |
| `ScriptShader.GL_ONE` | 1 | Factor: (1, 1, 1, 1) |
| `ScriptShader.GL_SRC_COLOR` | 768 | Factor: source colour |
| `ScriptShader.GL_ONE_MINUS_SRC_COLOR` | 769 | Factor: 1 - source colour |
| `ScriptShader.GL_SRC_ALPHA` | 770 | Factor: source alpha |
| `ScriptShader.GL_ONE_MINUS_SRC_ALPHA` | 771 | Factor: 1 - source alpha |
| `ScriptShader.GL_DST_ALPHA` | 772 | Factor: destination alpha |
| `ScriptShader.GL_ONE_MINUS_DST_ALPHA` | 773 | Factor: 1 - destination alpha |
| `ScriptShader.GL_DST_COLOR` | 774 | Factor: destination colour |
| `ScriptShader.GL_ONE_MINUS_DST_COLOR` | 775 | Factor: 1 - destination colour |
| `ScriptShader.GL_SRC_ALPHA_SATURATE` | 776 | Factor: min(As, 1-Ad) |

> OpenGL must be enabled in the application settings for shaders to render. Without an active OpenGL context, the shader area remains blank with no error message in exported plugins.

## Common Mistakes

- **Wrong:** Expecting `iMouse` to update automatically
  **Right:** Call `shader.setUniformData("iMouse", [x, y])` from the panel's mouse callback
  *The `iMouse` uniform is declared in the auto-prepended header but is not auto-populated - you must feed mouse coordinates manually via `setUniformData`.*

- **Wrong:** Calling `setPreprocessor` in the timer callback without change detection
  **Right:** Guard with `if (newLevel != currentLevel)` and call `setPreprocessor` only on change
  *Every `setPreprocessor` call recompiles the shader on the GPU. Calling it every frame destroys performance.*

- **Wrong:** Sending a raw Array with more than 4 elements as a uniform
  **Right:** Use `Buffer.create(size)` and pass the Buffer object
  *Arrays with more than 4 elements are silently ignored. Buffer is the correct type for variable-length float arrays in GLSL.*

- **Wrong:** Creating one shader object and switching its source at runtime
  **Right:** Create separate shader objects per visualisation mode and swap which one `g.applyShader()` renders
  *Shader compilation is expensive. Pre-create all variants and switch between them in the paint routine.*
