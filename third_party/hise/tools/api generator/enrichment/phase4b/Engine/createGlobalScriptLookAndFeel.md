Engine::createGlobalScriptLookAndFeel() -> ScriptObject

Thread safety: UNSAFE -- may allocate new ScriptedLookAndFeel or return existing singleton
Creates the global script look-and-feel, or returns the existing one if already created.
Singleton pattern -- first call creates with isGlobal=true and registers with
MainController; subsequent calls from any script processor return the same instance.
Anti-patterns:
  - Multiple processors can access the same global LAF instance. Registering a drawing
    function in one processor silently overwrites the previous registration for that
    function name from another processor -- last registration wins.
Pair with:
  Content.createLocalLookAndFeel -- per-component LAF override
Source:
  ScriptingApi.cpp  Engine::createGlobalScriptLookAndFeel()
    -> checks mc->getCurrentScriptLookAndFeel()
    -> returns existing or creates new ScriptedLookAndFeel(isGlobal=true)
