Sampler::setSortByRRGroup(Integer shouldSort) -> undefined

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD
Enables or disables sorting samples by round robin group for optimized voice
start. Recommended for large sample sets (>20,000 samples). When enabled,
activates the GroupedRoundRobinCollector which pre-sorts sounds into groups.
Pair with:
  enableRoundRobin -- related group management
  setActiveGroup -- group selection
Source:
  ScriptingApi.cpp  Sampler::setSortByRRGroup()
    -> activates GroupedRoundRobinCollector (ModulatorSampler.h:86)
