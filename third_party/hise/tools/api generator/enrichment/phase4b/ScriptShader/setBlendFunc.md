ScriptShader::setBlendFunc(Integer enabled, Integer sFactor, Integer dFactor) -> undefined

Thread safety: SAFE
Controls OpenGL alpha blending for shader output. When enabled, the source and
destination blend factors determine compositing with the existing framebuffer.
Previous GL blend state is saved before rendering and restored afterward.
Default factors: GL_SRC_ALPHA (source), GL_ONE_MINUS_SRC_ALPHA (destination).

Required setup:
  const var shd = Content.createShader("myEffect");
  shd.setBlendFunc(true, shd.GL_SRC_ALPHA, shd.GL_ONE);

Dispatch/mechanics:
  Stores enableBlending flag and src/dst BlendMode enum values.
  At render time (addShader draw action): saves current GL blend state,
  applies glBlendFunc(src, dst), renders shader, restores previous state.

Source:
  ScriptingGraphics.cpp  ScriptShader::setBlendFunc()
    -> stores enableBlending, src (BlendMode), dst (BlendMode)
  ScriptDrawActions.cpp:687  addShader draw action
    -> glGetIntegerv(GL_BLEND_SRC/GL_BLEND_DST) save
    -> glBlendFunc(src, dst) -> shader->fillRect() -> restore
