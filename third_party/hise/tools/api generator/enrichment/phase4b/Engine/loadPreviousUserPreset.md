Engine::loadPreviousUserPreset(bool stayInDirectory) -> undefined

Thread safety: UNSAFE -- file system access, preset loading
Loads the previous user preset. stayInDirectory=true stays within current subfolder.
Pair with:
  loadNextUserPreset -- navigate forward
  getCurrentUserPresetName -- query current preset
Source:
  ScriptingApi.cpp  Engine::loadPreviousUserPreset()
    -> UserPresetHandler::incPreset(false, stayInDirectory)
