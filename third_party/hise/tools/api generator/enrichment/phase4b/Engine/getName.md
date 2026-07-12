Engine::getName() -> String

Thread safety: WARNING -- string construction
Returns the product name from project settings. Backend reads HiseSettings,
frontend reads FrontendHandler.
Pair with:
  getVersion -- product version string
  getProjectInfo -- full metadata object
Source:
  ScriptingApi.cpp  Engine::getName()
    -> [backend] GET_HISE_SETTING(Project::Name)
    -> [frontend] FrontendHandler::getProjectName()
