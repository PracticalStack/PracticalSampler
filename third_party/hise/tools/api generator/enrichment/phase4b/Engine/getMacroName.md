Engine::getMacroName(int index) -> String

Thread safety: WARNING -- string construction, atomic ref-count
Returns the name of the macro at the given 1-based index (1-8).
Anti-patterns:
  - Do NOT use 0-based indexing -- macro indices are 1-based (1-8). Passing 0 reports
    "Illegal Macro Index"
Pair with:
  setFrontendMacros -- define macro names
Source:
  ScriptingApi.cpp  Engine::getMacroName()
    -> MacroControlBroadcaster::getMacroControlData(index-1)->getMacroName()
