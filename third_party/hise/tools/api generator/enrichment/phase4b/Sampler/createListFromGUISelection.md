Sampler::createListFromGUISelection() -> Array

Thread safety: UNSAFE -- USE_BACKEND only, acquires MessageManagerLock
Returns an array of Sample objects for the samples selected in the HISE sample
editor GUI. Returns an empty array silently in exported plugins.
Anti-patterns:
  - Do NOT rely on this in exported plugins -- returns empty array with no error
Pair with:
  setGUISelection -- set the GUI selection programmatically
  createSelection -- runtime alternative that works in exported plugins
Source:
  ScriptingApi.cpp  Sampler::createListFromGUISelection()
    -> guarded by #if USE_BACKEND
    -> reads from sample edit handler selection
    -> creates ScriptingSamplerSound objects
