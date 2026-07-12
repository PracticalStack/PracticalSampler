Settings::isOpenGLEnabled() -> Integer

Thread safety: SAFE
Returns whether OpenGL rendering is enabled. Reads the stored flag -- may not
reflect actual rendering state if setEnableOpenGL was called recently (deferred
until the next interface rebuild).

Pair with:
  setEnableOpenGL -- toggle OpenGL rendering

Source:
  ScriptingApi.cpp  Settings::isOpenGLEnabled()
    -> driver->useOpenGL (direct member read)
