Engine::createMacroHandler() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a macro handler for scripting access to HISE's macro control system (up to
HISE_NUM_MACROS slots, default 8). Allows programmatic macro connections, values,
and parameter queries. Macros must be enabled via setFrontendMacros() first.
Pair with:
  setFrontendMacros -- enable macros and set slot names
  getMacroName -- query macro slot names
Source:
  ScriptingApi.cpp  Engine::createMacroHandler()
    -> new ScriptedMacroHandler
