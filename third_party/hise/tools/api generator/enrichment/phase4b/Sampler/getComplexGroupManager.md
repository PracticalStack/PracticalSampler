Sampler::getComplexGroupManager() -> ScriptObject

Thread safety: UNSAFE -- allocates a new ScriptingComplexGroupManager object
Returns a ComplexGroupManager object for advanced group management beyond the
basic setActiveGroup/setMultiGroupIndex methods.
Pair with:
  enableRoundRobin -- must disable RR before using group management
  setActiveGroup -- simpler single-group alternative
  setMultiGroupIndex -- simpler multi-group alternative
Source:
  ScriptingApi.cpp  Sampler::getComplexGroupManager()
    -> new ScriptingObjects::ScriptingComplexGroupManager(getScriptProcessor(), sampler)
