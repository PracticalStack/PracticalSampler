Engine::createExpansionHandler() -> ScriptObject

Thread safety: UNSAFE -- heap allocation, dynamic_cast from script processor
Creates and activates the expansion handler for managing expansion packs. Registers
the calling script processor as an expansion listener.
Pair with:
  getExpansionList -- list available expansions
  setCurrentExpansion -- activate an expansion by name
Source:
  ScriptingApi.cpp  Engine::createExpansionHandler()
    -> new ScriptExpansionHandler(JavascriptProcessor*)
