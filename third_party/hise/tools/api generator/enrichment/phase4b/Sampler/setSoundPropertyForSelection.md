Sampler::setSoundPropertyForSelection(Integer propertyId, var newValue) -> undefined

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, callAsyncIfJobsPending
Sets a sound property to the same value for all sounds in the legacy script
selection (populated by selectSounds()). Uses async execution if jobs are pending.
Pair with:
  selectSounds -- populates the legacy selection
  setSoundPropertyForAllSamples -- apply to all samples instead
  refreshInterface -- call after to update UI
Source:
  ScriptingApi.cpp  Sampler::setSoundPropertyForSelection()
    -> s->callAsyncIfJobsPending(f)
