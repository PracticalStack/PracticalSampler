ScriptShader::setFragmentShader(String shaderFile) -> undefined

Thread safety: UNSAFE
Loads a GLSL fragment shader from the Scripts folder and compiles it. Specify
the filename without the .glsl extension. The engine auto-prepends built-in
uniforms, coordinate macros, and preprocessor definitions. Supports recursive
#include directives with circular dependency detection.

Required setup:
  const var shd = Content.createShader("");
  shd.setFragmentShader("myEffect");

Dispatch/mechanics:
  FileParser resolves shaderFile + ".glsl" from Scripts folder
    -> backend: registers ExternalScriptFile watcher for live reloading,
       creates default Shadertoy template if file missing
    -> frontend: loads from embedded script collection
  -> compileRawCode(loadedSource) -> builds shaderCode -> sets dirty flag

Pair with:
  setPreprocessor -- add #define directives before shader code
  fromBase64 -- alternative: load from embedded base64 string
  toBase64 -- encode loaded shader for distribution

Anti-patterns:
  - In frontend builds, if the shader file name does not match an embedded file
    in the script collection, the shader silently fails with no error message.

Source:
  ScriptingGraphics.cpp  ScriptShader::setFragmentShader()
  ScriptingGraphics.cpp:36  FileParser -- recursive #include resolution
  ScriptingGraphics.cpp:797  compileRawCode()
    -> preprocessors + getHeader() + code -> new OpenGLGraphicsContextCustomShader
