Sampler::getCurrentSampleMapId() -> String

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, returns String
Returns the ID string of the currently loaded sample map. Returns an empty
string if no sample map is loaded.
Pair with:
  loadSampleMap -- loads a map by ID
  getSampleMapList -- lists all available map IDs
Source:
  ScriptingApi.cpp  Sampler::getCurrentSampleMapId()
