Engine::isUserPresetReadOnly(var optionalFile) -> Integer

Thread safety: UNSAFE -- file system operations in frontend path
Checks if a user preset is read-only. Backend: always returns project setting
(ignores optionalFile). Frontend with READ_ONLY_FACTORY_PRESETS: checks specific
file. Frontend without: always returns false.
Anti-patterns:
  - In backend, optionalFile is completely ignored -- always returns global setting
Source:
  ScriptingApi.cpp  Engine::isUserPresetReadOnly()
    -> [backend] reads ReadOnlyFactoryPresets setting
    -> [frontend] UserPresetHandler::isReadOnly(file)
