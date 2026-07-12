Sampler::getSampleMapList() -> Array

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, allocates array and strings
Returns a sorted array of reference strings for all available sample maps in the
pool. Use these strings with loadSampleMap().
Pair with:
  loadSampleMap -- loads a map by reference string
  getCurrentSampleMapId -- gets the currently loaded map ID
Source:
  ScriptingApi.cpp  Sampler::getSampleMapList()
