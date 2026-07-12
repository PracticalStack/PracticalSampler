Sampler::getMicPositionName(Integer channelIndex) -> String

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, returns String
Returns the name (suffix) of a mic position channel by index. Only works with
multi-mic samplers (more than one mic position or static matrix enabled).
Anti-patterns:
  - Do NOT call on a single-mic sampler without static matrix -- reports a script error.
    Check getNumMicPositions() first.
Pair with:
  getNumMicPositions -- get valid index range
  purgeMicPosition -- purge by name (uses the string this method returns)
  isMicPositionPurged -- check purge state by index
Source:
  ScriptingApi.cpp  Sampler::getMicPositionName()
    -> s->getChannelData(channelIndex).suffix
