Engine::setFrontendMacros(var nameList) -> undefined

Thread safety: UNSAFE -- modifies macro manager state
Enables macro system and assigns names to each slot. Pass empty array to disable.
Anti-patterns:
  - Fewer names than HISE_NUM_MACROS (default 8) silently leaves excess slots
    with empty-string names -- provide names for all slots
  - Non-array argument silently disables macros and reports script error
Pair with:
  getMacroName -- query slot names
  createMacroHandler -- programmatic macro control
Source:
  ScriptingApi.cpp  Engine::setFrontendMacros()
    -> setEnableMacroOnFrontend(true) -> setMacroName() for each slot
