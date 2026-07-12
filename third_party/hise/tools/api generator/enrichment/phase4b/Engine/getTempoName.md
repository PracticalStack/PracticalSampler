Engine::getTempoName(int tempoIndex) -> String

Thread safety: WARNING -- string construction from static array lookup
Returns human-readable tempo name for the given index (e.g., 0="1/1", 5="1/4", 8="1/8").
Returns "Invalid" for out-of-range indices.
Pair with:
  getMilliSecondsForTempo -- duration for the same index
Source:
  ScriptingApi.cpp  Engine::getTempoName()
    -> TempoSyncer::getTempoName(tempoIndex)
