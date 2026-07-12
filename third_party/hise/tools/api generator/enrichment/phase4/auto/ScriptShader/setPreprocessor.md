Adds or updates a `#define` preprocessor directive in the shader source and triggers recompilation. The define is prepended before all built-in code, so it can be used for conditional compilation (`#ifdef`) or numeric constants in the GLSL source. Pass an empty string as the name to clear all preprocessor definitions.

```javascript
// Enable a quality tier in the GLSL code
shd.setPreprocessor("GRAPHICS_LEVEL", 2);

// The GLSL code can branch on it:
// #if GRAPHICS_LEVEL >= 2
//     // expensive path
// #else
//     // fast path
// #endif
```

> [!Warning:Every call recompiles the shader] Each `setPreprocessor` call triggers a full GPU shader recompile, even if the value has not changed. Always guard with a change check before calling, especially when applying the same define across multiple shaders in a loop.
