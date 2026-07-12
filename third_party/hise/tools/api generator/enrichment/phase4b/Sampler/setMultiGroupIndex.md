Sampler::setMultiGroupIndex(var groupIndex, Integer enabled) -> undefined

Thread safety: SAFE
Enables or disables one or more round robin groups simultaneously. Accepts a
single group index (Integer), an array of group indices, or a MidiList object.
Round robin must be disabled first.
Anti-patterns:
  - Do NOT call while round robin is enabled -- throws a script error
Pair with:
  setMultiGroupIndexForEventId -- per-event variant
  setActiveGroup -- simpler single-group alternative
  enableRoundRobin -- must disable RR first
Source:
  ScriptingApi.cpp  Sampler::setMultiGroupIndex()
    -> delegates to setMultiGroupIndexForEventId(-1, groupIndex, enabled)
