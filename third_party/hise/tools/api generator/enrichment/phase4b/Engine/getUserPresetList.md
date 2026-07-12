Engine::getUserPresetList() -> Array

Thread safety: UNSAFE -- filesystem I/O (recursive directory scan)
Returns array of all user preset paths as strings. Relative paths from UserPresets
root, without .preset extension, using forward slashes. Recursive scan.
Source:
  ScriptingApi.cpp  Engine::getUserPresetList()
    -> UserPresetHandler::findChildFiles(recursive) -> normalizes paths
