Sampler::isMicPositionPurged(Integer micIndex) -> Integer

Thread safety: SAFE
Returns whether a mic position channel is purged (disabled).
Anti-patterns:
  - Do NOT trust the return value for out-of-range indices -- returns false silently
    instead of reporting an error (known bug)
Pair with:
  purgeMicPosition -- purge/unpurge by name
  getNumMicPositions -- get valid index range
Source:
  ScriptingApi.cpp  Sampler::isMicPositionPurged()
    -> !s->getChannelData(micIndex).enabled
