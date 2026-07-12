Engine::loadFontAs(String fileName, String fontId) -> undefined

Thread safety: INIT -- file I/O in backend, runtime calls throw script error
Loads a font from Images folder and registers it under fontId. Platform-agnostic
replacement for deprecated loadFont(). No-op in frontend (fonts baked at export).
Anti-patterns:
  - Font file must be in the Images folder, not a custom Fonts folder
Pair with:
  setGlobalFont -- set the default font for the plugin
Source:
  ScriptingApi.cpp  Engine::loadFontAs()
    -> [backend] GET_PROJECT_HANDLER -> loadTypeFace(data, fontId)
    -> [frontend] no-op
