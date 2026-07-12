Engine::saveUserPreset(var presetName) -> undefined

Thread safety: UNSAFE -- file I/O, ValueTree serialization
Saves current plugin state as a user preset. Accepts string name or ScriptFile.
Empty string triggers a name prompt dialog.
Pair with:
  loadUserPreset -- load presets
  addModuleStateToUserPreset -- include module state
  createUserPresetHandler -- customize save lifecycle
Source:
  ScriptingApi.cpp  Engine::saveUserPreset()
    -> UserPresetHandler::savePreset() or UserPresetHelpers::saveUserPreset()
