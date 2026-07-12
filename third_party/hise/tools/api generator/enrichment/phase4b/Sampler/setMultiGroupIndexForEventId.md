Sampler::setMultiGroupIndexForEventId(Integer eventId, var groupIndex, Integer enabled) -> undefined

Thread safety: SAFE
Enables or disables one or more round robin groups for a specific event or
globally. Accepts a single group index, an array of indices, or a MidiList.
Pass -1 for eventId for global state.
Anti-patterns:
  - Do NOT call while round robin is enabled -- throws a script error
Pair with:
  setMultiGroupIndex -- convenience wrapper for global state (eventId=-1)
  setActiveGroupForEventId -- simpler single-group per-event alternative
  enableRoundRobin -- must disable RR first
Source:
  ScriptingApi.cpp  Sampler::setMultiGroupIndexForEventId()
    -> handles int, array, or MidiList for groupIndex
