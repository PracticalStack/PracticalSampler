Sampler::setActiveGroupForEventId(Integer eventId, Integer activeGroupIndex) -> undefined

Thread safety: SAFE -- but eventId != -1 requires audio thread (onNoteOn callback)
Sets the active round robin group for a specific event or globally. Pass -1 for
global group setting.
Anti-patterns:
  - Do NOT call with eventId != -1 outside onNoteOn -- throws "only available in
    onNoteOnCallback" error
  - Do NOT call while round robin is enabled -- throws a script error
Pair with:
  setActiveGroup -- convenience wrapper for global group (eventId=-1)
  getActiveRRGroupForEventId -- read the group back
  enableRoundRobin -- must disable RR first
Source:
  ScriptingApi.cpp  Sampler::setActiveGroupForEventId()
    -> explicit audio thread check for eventId != -1 via KillStateHandler
