Sampler::purgeMicPosition(String micName, Integer shouldBePurged) -> undefined

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, callAsyncIfJobsPending
Purges or unpurges a mic position channel by name. The name must match the
channel suffix string exactly (use getMicPositionName() to get valid names).
Only works with multi-mic samplers.
Anti-patterns:
  - Do NOT guess mic names -- use getMicPositionName() to get exact suffix strings.
    Reports "Channel not found" if the name does not match.
  - Do NOT call on a single-mic sampler without static matrix -- reports an error
Pair with:
  getMicPositionName -- get valid mic names
  isMicPositionPurged -- check current purge state
  getNumMicPositions -- get channel count
Source:
  ScriptingApi.cpp  Sampler::purgeMicPosition()
    -> matches micName against channel suffixes
    -> s->setMicEnabled(i, !shouldBePurged) via callAsyncIfJobsPending
