Sampler::getRRGroupsForMessage(Integer noteNumber, Integer velocity) -> Integer

Thread safety: SAFE
Returns the number of round robin groups that have samples mapped for the given
note number and velocity combination.
Anti-patterns:
  - Do NOT call without enableRoundRobin(false) first -- throws script error
  - Do NOT call without refreshRRMap() after loading a sample map -- returns stale data
Pair with:
  enableRoundRobin -- must disable RR first
  refreshRRMap -- must rebuild the RR map first
Source:
  ScriptingApi.cpp  Sampler::getRRGroupsForMessage()
