Sampler::refreshRRMap() -> undefined

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD
Rebuilds the internal round robin group map. Must be called after loading a
sample map and before using getRRGroupsForMessage().
Anti-patterns:
  - Do NOT call while round robin is enabled -- reports a script error.
    Call enableRoundRobin(false) first.
Pair with:
  getRRGroupsForMessage -- requires this method to be called first
  enableRoundRobin -- must disable RR before calling
Source:
  ScriptingApi.cpp  Sampler::refreshRRMap()
