Sampler::enableRoundRobin(Integer shouldUseRoundRobin) -> undefined

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD
Enables or disables automatic round robin group cycling. Must be disabled before
using manual group selection (setActiveGroup, setMultiGroupIndex, getRRGroupsForMessage).
Anti-patterns:
  - Do NOT call setActiveGroup or setMultiGroupIndex while round robin is enabled --
    throws a script error
Pair with:
  setActiveGroup -- single group selection (requires RR disabled)
  setMultiGroupIndex -- multi-group selection (requires RR disabled)
  getRRGroupsForMessage -- group queries (requires RR disabled)
Source:
  ScriptingApi.cpp  Sampler::enableRoundRobin()
    -> s->setUseRoundRobinLogic(shouldUseRoundRobin)
