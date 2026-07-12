Settings::setEnableOpenGL(Integer shouldBeEnabled) -> undefined

Thread safety: SAFE
Sets the OpenGL rendering flag. Does NOT immediately create or destroy the OpenGL
context -- the change takes effect on the next interface rebuild.

Anti-patterns:
  - Do NOT expect immediate visual change -- the OpenGL context switch is deferred.
    Always show a "please reload the plugin" message after toggling.
  - Persist the OpenGL state to a settings file (e.g., XML in AppData). HISE does
    not automatically persist this setting for exported plugins.

Pair with:
  isOpenGLEnabled -- read the current flag

Source:
  ScriptingApi.cpp  Settings::setEnableOpenGL()
    -> driver->useOpenGL = shouldBeEnabled (direct member write)
