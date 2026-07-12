Engine::setCurrentExpansion(String expansionName) -> Integer

Thread safety: UNSAFE -- expansion list iteration, notification dispatch
Sets the active expansion by name. Returns true if found and activated.
Empty string deactivates the current expansion.
Pair with:
  getExpansionList -- list available expansions
  createExpansionHandler -- full expansion management
Source:
  ScriptingApi.cpp  Engine::setCurrentExpansion()
    -> ExpansionHandler::setCurrentExpansion(expansionName)
