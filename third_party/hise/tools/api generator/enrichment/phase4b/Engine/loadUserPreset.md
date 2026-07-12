Engine::loadUserPreset(var relativePathOrFileObject) -> undefined

Thread safety: UNSAFE -- file system access, ValueTree parsing, preset restoration
Loads a user preset by relative path or ScriptFile. .preset extension auto-appended.
Anti-patterns:
  - Do NOT call during onInit -- explicitly rejected with script error
    "Do not load user presets at startup." Load from runtime callbacks instead.
Pair with:
  saveUserPreset -- save current state
  getUserPresetList -- list available presets
  createUserPresetHandler -- customize load/save lifecycle
Source:
  ScriptingApi.cpp  Engine::loadUserPreset()
    -> checks MainController::isInitialised() -> UserPresetHandler::loadUserPreset()
