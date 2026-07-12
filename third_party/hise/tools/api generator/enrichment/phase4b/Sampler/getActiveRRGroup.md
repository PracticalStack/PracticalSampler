Sampler::getActiveRRGroup() -> Integer

Thread safety: SAFE
Returns the currently active round robin group index (1-based).
Dispatch/mechanics:
  Delegates to getActiveRRGroupForEventId(-1) for global group state.
Pair with:
  getActiveRRGroupForEventId -- per-event variant
  setActiveGroup -- sets the active group
Source:
  ScriptingApi.cpp  Sampler::getActiveRRGroup()
    -> getActiveRRGroupForEventId(-1)
