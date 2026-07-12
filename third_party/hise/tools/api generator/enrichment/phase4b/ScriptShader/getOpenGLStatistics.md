ScriptShader::getOpenGLStatistics() -> JSON

Thread safety: SAFE
Returns a JSON object with GPU information: VersionString (String),
Major/Minor (int), Vendor (String), Renderer (String), GLSL Version (String).
Statistics are populated on the OpenGL render thread during the first shader
compilation pass.

Dispatch/mechanics:
  Returns cached openGLStats object. Populated by makeStatistics() on first
  render via glGetString(GL_VERSION/GL_VENDOR/GL_RENDERER) and
  OpenGLShaderProgram::getLanguageVersion().

Anti-patterns:
  - Do NOT call before the shader has rendered at least once -- returns
    undefined/placeholder values ("Inactive", zeros) until the GL context
    populates the statistics during the first GPU compilation pass.

Source:
  ScriptingGraphics.cpp:705  makeStatistics()
    -> glGetString(GL_VERSION), glGetIntegerv(GL_MAJOR_VERSION/GL_MINOR_VERSION)
    -> glGetString(GL_VENDOR), glGetString(GL_RENDERER)
    -> OpenGLShaderProgram::getLanguageVersion()
