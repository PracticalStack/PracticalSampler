Sampler::setSoundPropertyForAllSamples(Integer propertyIndex, var newValue) -> undefined

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, callAsyncIfJobsPending
Sets a sound property to the same value for ALL samples in the sampler (not just
the selection). Uses async execution if jobs are pending.
Pair with:
  setSoundPropertyForSelection -- apply to selection only
  setSoundProperty -- apply to a single sound
  refreshInterface -- call after to update UI
Source:
  ScriptingApi.cpp  Sampler::setSoundPropertyForAllSamples()
    -> iterates all sounds via SoundIterator
    -> s->callAsyncIfJobsPending(f)
