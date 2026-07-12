Engine::addModuleStateToUserPreset(var moduleId) -> undefined

Thread safety: UNSAFE -- modifies ModuleStateManager (heap allocations, processor tree traversal)
Adds a module's full ValueTree state to the user preset system. Passing an empty
string clears all registered modules. Also accepts a JSON object with an ID property.
Pair with:
  createUserPresetHandler -- customize preset save/load lifecycle
  saveUserPreset/loadUserPreset -- presets include registered module state
Anti-patterns:
  - Do NOT pass just a string when you need to filter state -- use a JSON object with
    RemovedProperties to exclude routing matrix and bypass state from the saved tree
Source:
  ScriptingApi.cpp  Engine::addModuleStateToUserPreset()
    -> ModuleStateManager::addProcessor() with processor lookup by ID
