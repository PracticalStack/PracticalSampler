Engine::loadNextUserPreset(bool stayInDirectory) -> undefined

Thread safety: UNSAFE -- file system access, preset loading
Loads the next user preset. stayInDirectory=true stays within current subfolder.
Pair with:
  loadPreviousUserPreset -- navigate backward
  getCurrentUserPresetName -- query current preset
Source:
  ScriptingApi.cpp  Engine::loadNextUserPreset()
    -> UserPresetHandler::incPreset(true, stayInDirectory)
