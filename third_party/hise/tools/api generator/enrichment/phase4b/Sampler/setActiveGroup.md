Sampler::setActiveGroup(Integer activeGroupIndex) -> undefined

Thread safety: SAFE
Sets a single active round robin group (1-based index). Round robin must be
disabled first.
Anti-patterns:
  - Do NOT call while round robin is enabled -- throws a script error
  - Do NOT pass an invalid group index -- throws a script error
Pair with:
  enableRoundRobin -- must disable RR first
  setActiveGroupForEventId -- per-event variant for onNoteOn
  setMultiGroupIndex -- enable multiple groups simultaneously
Source:
  ScriptingApi.cpp  Sampler::setActiveGroup()
    -> delegates to setActiveGroupForEventId(-1, activeGroupIndex)
