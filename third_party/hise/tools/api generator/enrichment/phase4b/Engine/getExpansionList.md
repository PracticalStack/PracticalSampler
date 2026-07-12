Engine::getExpansionList() -> Array

Thread safety: UNSAFE -- creates temporary ExpansionHandler, heap allocations
Returns an array of Expansion references for all currently available expansions.
Pair with:
  createExpansionHandler -- for persistent expansion management
  setCurrentExpansion -- activate an expansion by name
Source:
  ScriptingApi.cpp  Engine::getExpansionList()
    -> creates temp ScriptExpansionHandler -> getExpansionList()
