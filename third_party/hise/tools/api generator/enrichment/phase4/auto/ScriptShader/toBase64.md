Returns the current shader source as a base64-encoded, zstd-compressed string. The encoded string includes all files imported via `#include`, so the result is fully self-contained with no external dependencies.

The practical workflow for embedding a shader in a script:

```javascript
// 1. Load and test the shader from a .glsl file
const var shd = Content.createShader("myEffect");

// 2. Print the base64 string to the console
Console.print(shd.toBase64());

// 3. Copy the output and use fromBase64() in production
shd.fromBase64("copied-base64-string...");
```

This is useful for distributing shaders without requiring separate `.glsl` files - the entire shader can be embedded as a string literal in the script.
