ScriptShader::setPreprocessor(String name, NotUndefined value) -> undefined

Thread safety: UNSAFE
Adds or updates a #define preprocessor directive and triggers shader
recompilation. The define is prepended before all built-in uniforms and user
code. Passing an empty string as name clears all definitions. Value is
converted to string via toString().

Required setup:
  const var shd = Content.createShader("myEffect");
  shd.setPreprocessor("HIGH_QUALITY", 1);

Dispatch/mechanics:
  Empty name -> preprocessors.clear()
  Non-empty -> preprocessors.set(Identifier(name), value)
  -> compileRawCode(compiledCode) -- always recompiles with stored raw code

Pair with:
  setFragmentShader -- shader must be loaded before preprocessor definitions
    have any effect

Anti-patterns:
  - Do NOT call in a timer callback without change detection -- every call
    recompiles the shader on the GPU. Guard with
    if (newValue != currentValue) before calling.

Source:
  ScriptingGraphics.cpp:657  ScriptShader::setPreprocessor()
    -> preprocessors.set(Identifier(name), value)
    -> compileRawCode(compiledCode) -- full recompilation
