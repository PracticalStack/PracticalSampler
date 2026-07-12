ScriptShader::fromBase64(String b64) -> undefined

Thread safety: UNSAFE
Loads and compiles a shader from a base64-encoded, zstd-compressed GLSL string.
The string is decoded, decompressed, and compiled as a fragment shader (engine
auto-prepends built-in uniforms and coordinate macros).

Required setup:
  const var shd = Content.createShader("");

Dispatch/mechanics:
  Base64 decode -> zstd decompress -> compileRawCode(decompressed)
    -> builds shaderCode (preprocessors + header + user code)
    -> creates new OpenGLGraphicsContextCustomShader, sets dirty flag

Pair with:
  toBase64 -- encode current shader source for embedding in scripts
  setFragmentShader -- alternative: load from .glsl file instead of base64

Anti-patterns:
  - Do NOT pass arbitrary base64 strings -- must be produced by toBase64().
    Invalid strings silently fail with no shader compiled and no error reported.

Source:
  ScriptingGraphics.cpp  ScriptShader::fromBase64()
    -> Base64 decode + zstd decompress -> compileRawCode()
