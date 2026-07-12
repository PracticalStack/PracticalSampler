ScriptShader::setUniformData(String id, NotUndefined data) -> undefined

Thread safety: UNSAFE
Sets a custom uniform variable passed to the fragment shader every render frame.
Type mapping: double -> float, integer -> int, Array[2] -> vec2, Array[3] ->
vec3, Array[4] -> vec4, Buffer -> float[]. The uniform must be declared in the
GLSL source with a matching name and type.

Required setup:
  const var shd = Content.createShader("myEffect");
  shd.setUniformData("brightness", 0.8);

Dispatch/mechanics:
  Stores (id, data) in uniformData NamedValueSet. On each render frame,
  onShaderActivated lambda iterates uniformData and calls
  pr.setUniform(name, value) with type-appropriate overload (float, int,
  vec2/3/4, or float[] for Buffer).

Anti-patterns:
  - Do NOT set built-in uniforms iTime, uOffset, iResolution, or uScale --
    the engine overwrites these every frame. Only iMouse among built-ins can
    be user-controlled via this method.
  - Do NOT pass arrays with 1 or more than 4 elements -- silently ignored.
    Only sizes 2, 3, 4 map to vec2/vec3/vec4. Use Buffer for larger data.
  - Do NOT use a uniform name that doesn't match a GLSL declaration -- value
    is silently ignored with no error.

Source:
  ScriptingGraphics.cpp  ScriptShader::setUniformData()
    -> uniformData.set(Identifier(id), data)
  ScriptingGraphics.cpp:818  onShaderActivated lambda
    -> type dispatch: Double -> setUniform(float), Int -> setUniform(GLint),
       Array[2-4] -> setUniform(vec2/3/4), Buffer -> setUniform(float[], size)
