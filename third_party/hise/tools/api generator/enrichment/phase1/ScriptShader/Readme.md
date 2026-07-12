# ScriptShader -- Class Analysis

## Brief
OpenGL fragment shader wrapper for GPU-accelerated visual effects with Shadertoy-compatible uniforms and blend modes.

## Purpose
ScriptShader wraps a GLSL fragment shader for GPU-accelerated rendering within a ScriptPanel's paint routine. It provides Shadertoy-compatible built-in uniforms (iTime, iResolution, fragCoord, fragColor), custom uniform data passing (floats, vectors, integer, and Buffer arrays), OpenGL blend mode control, preprocessor definitions for shader variants, and a cached buffer system for screenshots. Shader files (.glsl) are loaded from the Scripts folder with recursive #include support, or can be embedded as base64-encoded strings for distribution.

## Details

### Rendering Pipeline

ScriptShader does not render independently -- it requires a hosting ScriptPanel. The workflow is:

1. Create a shader with `Content.createShader("myShader")` (loads `myShader.glsl` from Scripts folder)
2. In the panel's paint routine, call `g.applyShader(shader, [x, y, w, h])` to fill a rectangle with the shader output
3. Call `panel.repaint()` to trigger re-rendering (e.g., from a timer for animations)

OpenGL must be enabled in the application settings for shaders to render. Without OpenGL, the shader area remains blank.

### Shader Code Structure

User-written GLSL code is a fragment shader `main()` function. The engine automatically prepends:

- **Uniforms:** `uScale` (float), `iTime` (float), `iMouse` (vec2), `uOffset` (vec2), `iResolution` (vec3)
- **Macros:** `fragCoord` (pixel coordinates with Y-flip and scale correction), `fragColor` (output color alias for `gl_FragColor`)
- **From JUCE:** `pixelAlpha` (opacity uniform from the OpenGL context)

User preprocessor definitions (via `setPreprocessor`) are prepended before all built-in code.

### Uniform Data Types

`setUniformData` accepts these HiseScript-to-GLSL mappings:

| HiseScript Value | GLSL Type |
|-----------------|-----------|
| Number (double) | float |
| Integer | int |
| Array (2 elements) | vec2 |
| Array (3 elements) | vec3 |
| Array (4 elements) | vec4 |
| Buffer | float[] |

See `setUniformData()` for usage examples and constraints.

### Blend Modes

`setBlendFunc()` controls OpenGL alpha blending using the GL_* constants exposed on the shader object. See `setBlendFunc()` for blend factor parameters and save/restore behavior.

### File Include System

GLSL files support recursive `#include` directives. See `setFragmentShader()` for include resolution, circular dependency detection, and live reloading details.

### Cached Buffer System

See `setEnableCachedBuffer()` for the GPU-to-CPU readback system that enables `Content.createScreenshot()` to capture shader output.

## obtainedVia
`Content.createShader(shaderFileName)`

## minimalObjectToken
shd

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| GL_ZERO | 0 | int | Blend factor: (0, 0, 0, 0) | BlendMode |
| GL_ONE | 1 | int | Blend factor: (1, 1, 1, 1) | BlendMode |
| GL_SRC_COLOR | 768 | int | Blend factor: source color | BlendMode |
| GL_ONE_MINUS_SRC_COLOR | 769 | int | Blend factor: 1 - source color | BlendMode |
| GL_SRC_ALPHA | 770 | int | Blend factor: source alpha | BlendMode |
| GL_ONE_MINUS_SRC_ALPHA | 771 | int | Blend factor: 1 - source alpha | BlendMode |
| GL_DST_ALPHA | 772 | int | Blend factor: destination alpha | BlendMode |
| GL_ONE_MINUS_DST_ALPHA | 773 | int | Blend factor: 1 - destination alpha | BlendMode |
| GL_DST_COLOR | 774 | int | Blend factor: destination color | BlendMode |
| GL_ONE_MINUS_DST_COLOR | 775 | int | Blend factor: 1 - destination color | BlendMode |
| GL_SRC_ALPHA_SATURATE | 776 | int | Blend factor: min(As, 1-Ad) | BlendMode |

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using `g.applyShader(shader, area)` without OpenGL enabled | Enable OpenGL in Settings before using shaders | Without an active OpenGL context, the shader cannot compile or render -- the area remains blank with no error in frontend builds |
| Expecting `iMouse` to update automatically | Call `shader.setUniformData("iMouse", [x, y])` from the panel's mouse callback | The `iMouse` uniform is declared in the header but not auto-populated -- the user must feed mouse coordinates manually |

## codeExample
```javascript
const var pnl = Content.addPanel("ShaderPanel", 0, 0);
pnl.set("width", 512);
pnl.set("height", 512);

const var shd = Content.createShader("myEffect");

pnl.setPaintRoutine(function(g)
{
    g.applyShader(shd, [0, 0, this.getWidth(), this.getHeight()]);
});

pnl.startTimer(30);
pnl.setTimerCallback(function()
{
    this.repaint();
});
```

## Alternatives
- Graphics: CPU-based 2D drawing operations without GPU acceleration
- ScriptPanel: hosts the paint routine that applies the shader via Graphics.applyShader()
- Buffer: can be passed as uniform data to the shader for GPU-accelerated data visualization

## Related Preprocessors
`USE_BACKEND`, `HISE_SUPPORT_GLSL_LINE_NUMBERS`

## Diagnostic Ideas
Reviewed: Yes
Count: 1
- ScriptShader.setUniformData -- value-check (logged)
