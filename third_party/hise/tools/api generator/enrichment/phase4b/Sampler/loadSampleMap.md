Sampler::loadSampleMap(String fileName) -> undefined

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, killAllVoicesAndCall
Loads a sample map by its pool reference string. Kills all active voices before
loading. The reference string should match the format returned by getSampleMapList().
Anti-patterns:
  - Do NOT call directly from a ComboBox control callback -- use a timer to defer
    (50ms poll) to avoid audio glitches during preloading
  - Do NOT pass an empty string -- reports a script error
Pair with:
  getSampleMapList -- get available map reference strings
  getCurrentSampleMapId -- check what is currently loaded
  clearSampleMap -- clear before loading if needed
Source:
  ScriptingApi.cpp  Sampler::loadSampleMap()
    -> s->killAllVoicesAndCall(..., true)
