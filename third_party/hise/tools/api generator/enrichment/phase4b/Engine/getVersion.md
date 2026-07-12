Engine::getVersion() -> String

Thread safety: UNSAFE -- String construction from settings/handler
Returns the product version string (e.g., "1.0.0"). Not the HISE engine version.
Pair with:
  getName -- product name
  getProjectInfo -- full metadata including HISEBuild
Source:
  ScriptingApi.cpp  Engine::getVersion()
    -> [backend] HiseSettings::Project::Version
    -> [frontend] FrontendHandler::getVersionString()
