UserPresetHandler::isOldVersion(String version) -> Integer

Thread safety: WARNING -- accesses project settings (backend) or FrontendHandler::getVersionString() (frontend); involves String construction
Compares the given version string against the current project version. Returns
true if the given version is older. Uses SemanticVersionChecker with
major.minor.patch format.
Dispatch/mechanics:
  Backend: reads HiseSettings::Project::Version from GlobalSettingManager
  Frontend: reads FrontendHandler::getVersionString()
  SemanticVersionChecker(version, thisVersion).isUpdate()
Pair with:
  setPreCallback -- typically called inside the pre-callback for migration
  setEnableUserPresetPreprocessing -- enables JSON access to preset version
Source:
  ScriptExpansion.cpp:247  isOldVersion()
    -> SemanticVersionChecker(version, thisVersion).isUpdate()
