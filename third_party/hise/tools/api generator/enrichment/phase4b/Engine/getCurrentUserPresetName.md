Engine::getCurrentUserPresetName() -> String

Thread safety: WARNING -- string construction, atomic ref-count operations
Returns the filename (without .preset extension) of the currently loaded user preset.
Returns empty string if no preset has been loaded. Bare filename only, not full path.
Anti-patterns:
  - Do NOT use for disambiguation if presets in different folders share names --
    use getUserPresetList() for full relative paths
Source:
  ScriptingApi.cpp  Engine::getCurrentUserPresetName()
    -> UserPresetHandler::getCurrentlyLoadedFile() -> getFileNameWithoutExtension()
