Sampler::setUseStaticMatrix(Integer shouldUseStaticMatrix) -> undefined

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD
Enables or disables the static routing matrix for this sampler. When enabled,
the sampler uses a fixed channel routing matrix instead of the dynamic mic
position system.
Pair with:
  getNumMicPositions -- affected by matrix mode
  purgeMicPosition -- mic management changes with matrix mode
Source:
  ScriptingApi.cpp  Sampler::setUseStaticMatrix()
