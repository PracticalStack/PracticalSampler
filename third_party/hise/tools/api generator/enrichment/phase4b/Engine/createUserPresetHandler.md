Engine::createUserPresetHandler() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a user preset handler for customizing the preset lifecycle: intercepting
load/save events, implementing custom preset models, attaching pre/post callbacks,
and querying/modifying preset state.
Pair with:
  loadUserPreset/saveUserPreset -- trigger preset operations
  getCurrentUserPresetName -- query current preset
  addModuleStateToUserPreset -- include module state in presets
Source:
  ScriptingApi.cpp  Engine::createUserPresetHandler()
    -> new ScriptUserPresetHandler
