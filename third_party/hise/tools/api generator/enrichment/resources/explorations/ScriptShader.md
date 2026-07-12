# ScriptShader -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` -- ScriptShader entry
- `enrichment/phase1/ScriptPanel/Readme.md` -- prerequisite class context
- `enrichment/base/ScriptShader.json` -- base API method list

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingGraphics.h:234`

```cpp
class ScriptShader : public ConstScriptingObject,
                     public ScreenshotListener
```

- Inherits from `ConstScriptingObject` (standard scripting API base) and `ScreenshotListener` (for cached buffer / screenshot support).
- Lives inside `ScriptingObjects` namespace (nested in `ScriptingGraphics.h`).
- Object name: `"ScriptShader"` (from `RETURN_STATIC_IDENTIFIER`).
- Weak-referenceable via `JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptShader)`.

## ScreenshotListener Base Class

**File:** `ScriptingGraphics.h:37-64`

```cpp
struct ScreenshotListener
{
    struct CachedImageBuffer : public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<CachedImageBuffer>;
        CachedImageBuffer(Rectangle<int> sb) :
            data(Image::RGB, sb.getWidth(), sb.getHeight(), true) {}
        Image data;
    };

    virtual ~ScreenshotListener() {};
    virtual void makeScreenshot(const File& targetFile, Rectangle<float> area) {};
    virtual void prepareScreenshot() {};
    virtual int blockWhileWaiting() { return 0; };
    virtual void visualGuidesChanged() {};
};
```

ScriptShader overrides `prepareScreenshot()` and `blockWhileWaiting()` for the cached buffer pipeline.

## Factory Method (obtainedVia)

**File:** `ScriptingApiContent.cpp:8753-8768`

```cpp
var ScriptingApi::Content::createShader(const String& fileName)
{
    auto f = new ScriptingObjects::ScriptShader(getScriptProcessor());
    addScreenshotListener(f);
#if HISE_SUPPORT_GLSL_LINE_NUMBERS
    f->setEnableLineNumbers(true);
#endif
    if(!fileName.isEmpty())
        f->setFragmentShader(fileName);
    return var(f);
}
```

Created via `Content.createShader(fileName)`. The fileName parameter is optional -- if non-empty, immediately loads and compiles the specified .glsl file. The shader is registered as a screenshot listener on the Content object.

## Constructor and Constants

**File:** `ScriptingGraphics.cpp:474-500`

```cpp
ScriptShader(ProcessorWithScriptingContent* sp) :
    ConstScriptingObject(sp, (int)BlendMode::numBlendModes),
    r(Result::fail("uncompiled"))
{
    using namespace juce::gl;
    addConstant("GL_ZERO", GL_ZERO);
    addConstant("GL_ONE", GL_ONE);
    addConstant("GL_SRC_COLOR", GL_SRC_COLOR);
    addConstant("GL_ONE_MINUS_SRC_COLOR", GL_ONE_MINUS_SRC_COLOR);
    addConstant("GL_DST_COLOR", GL_DST_COLOR);
    addConstant("GL_ONE_MINUS_DST_COLOR", GL_ONE_MINUS_DST_COLOR);
    addConstant("GL_SRC_ALPHA", GL_SRC_ALPHA);
    addConstant("GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA);
    addConstant("GL_DST_ALPHA", GL_DST_ALPHA);
    addConstant("GL_ONE_MINUS_DST_ALPHA", GL_ONE_MINUS_DST_ALPHA);
    addConstant("GL_SRC_ALPHA_SATURATE", GL_SRC_ALPHA_SATURATE);

    ADD_API_METHOD_1(setFragmentShader);
    ADD_API_METHOD_2(setUniformData);
    ADD_API_METHOD_3(setBlendFunc);
    ADD_API_METHOD_1(fromBase64);
    ADD_API_METHOD_0(toBase64);
    ADD_API_METHOD_0(getOpenGLStatistics);
    ADD_API_METHOD_1(setEnableCachedBuffer);
    ADD_API_METHOD_2(setPreprocessor);
}
```

### Constants Table

All 11 constants are OpenGL blend factor values used with `setBlendFunc()`:

| Name | GL Enum Value | Description |
|------|---------------|-------------|
| GL_ZERO | 0 | Factor (0, 0, 0, 0) |
| GL_ONE | 1 | Factor (1, 1, 1, 1) |
| GL_SRC_COLOR | 0x0300 | Source color |
| GL_ONE_MINUS_SRC_COLOR | 0x0301 | 1 - source color |
| GL_DST_COLOR | 0x0306 | Destination color |
| GL_ONE_MINUS_DST_COLOR | 0x0307 | 1 - destination color |
| GL_SRC_ALPHA | 0x0302 | Source alpha |
| GL_ONE_MINUS_SRC_ALPHA | 0x0303 | 1 - source alpha |
| GL_DST_ALPHA | 0x0304 | Destination alpha |
| GL_ONE_MINUS_DST_ALPHA | 0x0305 | 1 - destination alpha |
| GL_SRC_ALPHA_SATURATE | 0x0308 | min(As, 1-Ad) |

**Note:** All methods use `ADD_API_METHOD_N` (untyped), NOT `ADD_TYPED_API_METHOD_N`. No forced parameter types.

Initial state: `r = Result::fail("uncompiled")` -- shader starts in error state until code is loaded.

## BlendMode Enum

**File:** `ScriptingGraphics.h:264-278`

```cpp
enum class BlendMode
{
    _GL_ZERO = juce::gl::GL_ZERO,
    _GL_ONE = juce::gl::GL_ONE,
    _GL_SRC_COLOR = juce::gl::GL_SRC_COLOR,
    _GL_ONE_MINUS_SRC_COLOR = juce::gl::GL_ONE_MINUS_SRC_COLOR,
    _GL_DST_COLOR = juce::gl::GL_DST_COLOR,
    _GL_ONE_MINUS_DST_COLOR = juce::gl::GL_ONE_MINUS_DST_COLOR,
    _GL_SRC_ALPHA = juce::gl::GL_SRC_ALPHA,
    _GL_ONE_MINUS_SRC_ALPHA = juce::gl::GL_ONE_MINUS_SRC_ALPHA,
    _GL_DST_ALPHA = juce::gl::GL_DST_ALPHA,
    _GL_ONE_MINUS_DST_ALPHA = juce::gl::GL_ONE_MINUS_DST_ALPHA,
    _GL_SRC_ALPHA_SATURATE = juce::gl::GL_SRC_ALPHA_SATURATE,
    numBlendModes = 11
};
```

Maps directly to OpenGL blend factor constants. Used by `setBlendFunc()` to cast integer arguments to `BlendMode` enum values.

## Wrapper Struct (Method Registration)

**File:** `ScriptingGraphics.cpp:462-472`

All wrappers use standard `API_VOID_METHOD_WRAPPER_N` / `API_METHOD_WRAPPER_N` -- no typed wrappers:

```cpp
API_VOID_METHOD_WRAPPER_1(ScriptShader, setFragmentShader);
API_VOID_METHOD_WRAPPER_2(ScriptShader, setUniformData);
API_VOID_METHOD_WRAPPER_3(ScriptShader, setBlendFunc);
API_VOID_METHOD_WRAPPER_1(ScriptShader, fromBase64);
API_VOID_METHOD_WRAPPER_1(ScriptShader, setEnableCachedBuffer);
API_VOID_METHOD_WRAPPER_2(ScriptShader, setPreprocessor);
API_METHOD_WRAPPER_0(ScriptShader, toBase64);
API_METHOD_WRAPPER_0(ScriptShader, getOpenGLStatistics);
```

## FileParser Inner Class

**File:** `ScriptingGraphics.h:241-262`, implementation at `ScriptingGraphics.cpp:36-154`

Handles loading .glsl files from the Scripts folder with recursive `#include` support.

### Backend behavior (`USE_BACKEND`):
- Resolves file from `FileHandlerBase::Scripts` directory with `.glsl` extension
- Registers file with the ExternalScriptFile watcher system for live reloading
- Adds shader file to the script engine via `addShaderFile()`
- If file doesn't exist, creates a default Shadertoy-style template:
```glsl
void main()
{
    vec2 uv = fragCoord/iResolution.xy;
    vec3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));
    fragColor = pixelAlpha * vec4(col,1.0);
}
```
- Supports `#line` directives for error mapping (when `EnableShaderLineNumbers` setting is on)
- Detects circular includes: throws if same file is included twice

### Frontend behavior:
- Loads from embedded script collection via `getExternalScriptFromCollection()`
- Appends `.glsl` extension if not present

## Shader Header (Auto-injected Uniforms)

**File:** `ScriptingGraphics.cpp:567-596`

```cpp
String ScriptShader::getHeader()
{
    String s;
    s << "uniform float uScale;";
    s << "uniform float iTime;";
    s << "uniform vec2 iMouse;";
    s << "uniform vec2 uOffset;";
    s << "uniform vec3 iResolution;";
    s << "";
    s << "vec2 _gl_fc()";
    s << "{";
    s << "vec2 p = vec2(pixelPos.x + uOffset.x,";
    s << "  pixelPos.y + uOffset.y) / uScale;";
    s << "p.y = iResolution.y - p.y;";
    s << "return p;";
    s << "}";
    s << "\n#define fragCoord _gl_fc()\n";
    s << "#define fragColor gl_FragColor\n";
    // Backend only: #line directive for shader name
}
```

### Built-in Uniforms

| Uniform | GLSL Type | Description |
|---------|-----------|-------------|
| `uScale` | float | Display scale factor |
| `iTime` | float | Time since shader compilation in seconds |
| `iMouse` | vec2 | Mouse position (user-set via setUniformData) |
| `uOffset` | vec2 | Global position offset for coordinate mapping |
| `iResolution` | vec3 | Panel dimensions (width, height, 1.0) |

### Built-in Macros

| Macro | Expands To | Description |
|-------|-----------|-------------|
| `fragCoord` | `_gl_fc()` | Normalized pixel coordinates (Shadertoy-compatible) |
| `fragColor` | `gl_FragColor` | Output color (Shadertoy-compatible) |

The `_gl_fc()` function transforms from OpenGL pixel coordinates to Shadertoy-style coordinates, accounting for scale factor and Y-axis flip.

**Note:** `pixelAlpha` is also available (referenced in default template) -- this comes from `OpenGLGraphicsContextCustomShader`'s built-in uniform, not from the ScriptShader header.

## Shader Compilation Pipeline

**File:** `ScriptingGraphics.cpp:797-875`

`compileRawCode(const String& code)`:

1. Stores raw code in `compiledCode` (for base64 serialization and preprocessor recompilation)
2. Builds `shaderCode`: preprocessor defines + header + compiled code
3. Creates new `OpenGLGraphicsContextCustomShader` from the combined code
4. Sets `iTime` to current millisecond counter (for time-based animations)
5. Registers `onShaderActivated` lambda that runs every frame:
   - Updates `iTime` uniform (elapsed time since compilation)
   - Updates `uOffset` (global-to-local coordinate mapping)
   - Updates `iResolution` (panel dimensions)
   - Updates `uScale` (scale factor)
   - Iterates all user-set uniforms and sends to GPU

### Uniform Type Mapping (onShaderActivated lambda)

The lambda at `ScriptingGraphics.cpp:818-872` maps HiseScript types to OpenGL uniforms:

| HiseScript Type | GLSL Type | Notes |
|----------------|-----------|-------|
| Double (float) | float | `pr.setUniform(name, (float)v)` |
| Int/Int64 | int | Cast via reinterpret_cast to GLint |
| Array[2] | vec2 | `pr.setUniform(name, v[0], v[1])` |
| Array[3] | vec3 | `pr.setUniform(name, v[0], v[1], v[2])` |
| Array[4] | vec4 | `pr.setUniform(name, v[0], v[1], v[2], v[3])` |
| Buffer | float[] | `pr.setUniform(name, ptr, size)` -- static float array |

**Key detail:** `iMouse` is listed in the header as a built-in uniform but is NOT automatically populated by the engine. The user must call `setUniformData("iMouse", [x, y])` manually (typically from a mouse callback on the hosting ScriptPanel).

## Graphics.applyShader Integration

**File:** `ScriptingGraphics.cpp:2379-2389`

```cpp
bool GraphicsObject::applyShader(var shader, var area)
{
    if (auto obj = dynamic_cast<ScriptingObjects::ScriptShader*>(shader.getObject()))
    {
        Rectangle<int> b = getRectangleFromVar(area).toNearestInt();
        drawActionHandler.addDrawAction(new ScriptedDrawActions::addShader(&drawActionHandler, obj, b));
        return true;
    }
    return false;
}
```

This is the bridge between ScriptPanel's paint routine and the shader. Inside a panel's paint callback, call `g.applyShader(shader, [x, y, w, h])` to render the shader into the specified rectangle.

## addShader Draw Action

**File:** `ScriptDrawActions.cpp:687-826`

The `addShader` draw action handles the actual OpenGL rendering:

1. **Screenshot cache check:** If `getScreenshotBuffer()` returns a cached image, draws that instead of running the shader
2. **Compilation check:** On first render after code change (`dirty` flag), calls `shader->checkCompilation()` and updates statistics
3. **Error handling (backend only):** Clears GL errors and logs compilation errors via `handler->logError()`
4. **Blend mode setup:** Saves current GL blend state, applies custom blend mode if enabled, restores after render
5. **Rendering:** Calls `shader->fillRect()` to execute the fragment shader
6. **Buffer capture:** If `shouldWriteToBuffer()` is true (cache enabled or screenshot pending):
   - Gets screenshot bounds from handler
   - Creates `CachedImageBuffer` (RGB image)
   - Reads pixels from GPU via `glReadPixels(GL_BGR_EXT, GL_UNSIGNED_BYTE)`
   - Flips image vertically (OpenGL Y-axis is bottom-up)
7. **Notifies shader:** Calls `renderWasFinished(cachedOpenGlBuffer)`

## Cached Buffer / Screenshot System

### enableCache mode (`setEnableCachedBuffer(true)`):
- `shouldWriteToBuffer()` returns true on every frame
- After each render, `glReadPixels` captures the GPU output
- On subsequent renders, if a cached buffer exists and `isRenderingScreenshot()` is true, the cached image is drawn instead of re-running the shader

### Screenshot flow:
1. `Content.createScreenshot()` calls `prepareScreenshot()` on all listeners
2. `prepareScreenshot()` sets `screenshotPending = true` (only if compiled OK and cache enabled)
3. Next render captures pixels into `lastScreenshot`
4. `blockWhileWaiting()` polls for up to 2000ms until screenshot completes
5. `renderWasFinished()` clears `screenshotPending` and stores the buffer

## Member Variables

**File:** `ScriptingGraphics.h:337-389`

| Member | Type | Purpose |
|--------|------|---------|
| `lastScreenshot` | CachedImageBuffer::Ptr | Last captured screenshot |
| `scaleFactor` | float (1.0) | Display scale |
| `shaderCode` | String | Final compiled code (header + preprocessors + user code) |
| `uniformData` | NamedValueSet | User-set uniform variables |
| `openGLStats` | var | JSON object with GL statistics |
| `shader` | ScopedPointer<OpenGLGraphicsContextCustomShader> | JUCE shader wrapper |
| `dirty` | bool | Needs recompilation check |
| `useLineNumbers` | bool | Enable #line directives |
| `iTime` | double | Timestamp at compilation (for elapsed time calc) |
| `globalRect` | Rectangle<float> | Global bounds on screen |
| `localRect` | Rectangle<float> | Local bounds within panel |
| `includedFiles` | ReferenceCountedArray<ExternalScriptFile> | Tracked .glsl files |
| `enableBlending` | bool | Blend mode enabled |
| `enableCache` | bool | Cache buffer enabled |
| `src` | BlendMode (GL_SRC_ALPHA) | Source blend factor |
| `dst` | BlendMode (GL_ONE_MINUS_SRC_ALPHA) | Destination blend factor |
| `preprocessors` | NamedValueSet (private) | Preprocessor definitions |
| `screenshotPending` | bool (private) | Waiting for screenshot capture |
| `renderingScreenShot` | static bool (private) | Global screenshot rendering flag |
| `compiledCode` | String (private) | Raw user code (without header/preprocessors) |
| `shaderName` | String (private) | .glsl filename for error messages |
| `r` | Result (private) | Compilation result |

## OpenGL Statistics

**File:** `ScriptingGraphics.cpp:705-763`

`makeStatistics()` queries OpenGL for GPU information and builds a JSON object:

| Property | Source |
|----------|--------|
| VersionString | `glGetString(GL_VERSION)` |
| Major | `glGetIntegerv(GL_MAJOR_VERSION)` (with string fallback) |
| Minor | `glGetIntegerv(GL_MINOR_VERSION)` (with string fallback) |
| Vendor | `glGetString(GL_VENDOR)` |
| Renderer | `glGetString(GL_RENDERER)` |
| GLSL Version | `OpenGLShaderProgram::getLanguageVersion()` |

If no OpenGL context is active, returns placeholder with "Inactive" strings and zero versions.

## PreviewComponent (Backend Only)

**File:** `ScriptingGraphics.cpp:156-459`

Backend-only debug component (`USE_BACKEND`):
- Live shader preview with 15ms timer-driven repaint
- Toolbar: stats toggle (shows JSON stats overlay), view toggle (shows uniform data table), reset time button
- UniformProvider subclass that feeds uniform names/values into a ScriptWatchTable
- Checks for OpenGL enabled state and shows error message if not
- Full blend mode support in preview

## Preprocessor System

**File:** `ScriptingGraphics.cpp:657-665`

```cpp
void ScriptShader::setPreprocessor(String preprocessorString, var value)
{
    if (preprocessorString.isEmpty())
        preprocessors.clear();
    else
        preprocessors.set(Identifier(preprocessorString), value);
    compileRawCode(compiledCode);
}
```

- Empty string clears all preprocessors
- Non-empty adds/updates a `#define name value` line
- Always recompiles the shader (calls `compileRawCode` with stored `compiledCode`)
- Preprocessor defines are prepended BEFORE the header in the final shader code

## Build Target Differences

| Feature | Backend (USE_BACKEND) | Frontend |
|---------|----------------------|----------|
| File loading | From Scripts folder, creates default if missing | From embedded collection |
| File watching | Registers ExternalScriptFile watchers | Not available |
| #line directives | Conditional on HiseSettings | Not available |
| Error logging | GL error drain + error log | Silent |
| Preview component | Full shader preview | Returns nullptr |

## Prerequisite Integration (ScriptPanel)

From the ScriptPanel Readme:
- ScriptShader operates within a ScriptPanel's paint routine
- The workflow is: create shader via `Content.createShader()`, then in the panel's paint callback call `g.applyShader(shader, area)`
- The panel's `repaint()` triggers re-rendering of the shader
- OpenGL must be enabled in the application settings for shaders to work
- The panel provides the rendering area and coordinate system; the shader provides the GPU code

## Threading Notes

- Shader compilation happens lazily: `compileRawCode()` creates the shader object and sets `dirty = true`, but actual GPU compilation happens on the next render via `checkCompilation()` (called from the OpenGL render thread in the draw action)
- `onShaderActivated` lambda runs on the OpenGL render thread every frame
- `blockWhileWaiting()` blocks the scripting thread polling for screenshot completion (up to 2000ms timeout)
- Uniform data can be set from any thread (stored in `NamedValueSet`, read on GL thread)
