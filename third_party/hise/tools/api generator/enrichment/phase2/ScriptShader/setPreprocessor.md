## setPreprocessor

**Examples:**

```javascript:quality-level-broadcast
// Title: Graphics quality setting that controls all shaders
// Context: A settings control lets the user choose a GPU quality level.
//          All active shaders receive the same preprocessor define, which
//          their GLSL code uses for #ifdef quality branching.

const var shaderA = Content.createShader("effectA");
const var shaderB = Content.createShader("effectB");
const var shaderC = Content.createShader("effectC");

const var allShaders = [shaderA, shaderB, shaderC];

reg currentQuality = -1;

// Called when the user changes the graphics quality setting
inline function setGraphicsQuality(level)
{
    // Guard: every call recompiles all shaders on the GPU
    if (level == currentQuality)
        return;

    currentQuality = level;

    for (shd in allShaders)
        shd.setPreprocessor("GRAPHICS_LEVEL", level);
}

// In the GLSL shader, use the define for conditional quality:
//
//   #if GRAPHICS_LEVEL >= 3
//       // expensive ray-marching loop
//   #elif GRAPHICS_LEVEL >= 1
//       // simplified approximation
//   #else
//       // minimal fallback
//   #endif
```
```json:testMetadata:quality-level-broadcast
{
  "testable": false,
  "skipReason": "Requires shader files and active OpenGL context for compilation"
}
```

**Pitfalls:**
- Every `setPreprocessor` call triggers a full GPU shader recompile, even if the value hasn't changed. When applying the same define to multiple shaders (e.g., a quality level), always guard with a change check before the loop.
