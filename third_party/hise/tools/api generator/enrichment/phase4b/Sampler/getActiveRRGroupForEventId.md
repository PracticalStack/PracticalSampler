Sampler::getActiveRRGroupForEventId(Integer eventId) -> Integer

Thread safety: SAFE
Returns the active round robin group for a specific event. Pass -1 for global state.
Pair with:
  getActiveRRGroup -- convenience wrapper using eventId=-1
  setActiveGroupForEventId -- sets the group for a specific event
Source:
  ScriptingApi.cpp  Sampler::getActiveRRGroupForEventId()
