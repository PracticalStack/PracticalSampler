Unlocker::getLicenseKeyFile() -> String

Thread safety: WARNING -- String involvement, atomic ref-count operations on the returned path string.
Returns the full file path of the license key file. Backend: {AppDataRoot}/{Company}/{Project}/{Project}.{ext}.
Frontend: delegates to FrontendHandler::getLicenseKey(). Returns expected path regardless of whether
the file exists -- use keyFileExists() to check.

Pair with:
  keyFileExists -- check if the file at this path actually exists
  writeKeyFile -- writes key data to this location
  loadKeyFile -- reads and validates from this location

Source:
  ScriptExpansion.cpp  ScriptUnlocker::getLicenseKeyFile()
    -> USE_BACKEND: ProjectHandler::getAppDataRoot().getChildFile(company/project/project).withFileExtension(ext)
    -> USE_FRONTEND: FrontendHandler::getLicenseKey()
