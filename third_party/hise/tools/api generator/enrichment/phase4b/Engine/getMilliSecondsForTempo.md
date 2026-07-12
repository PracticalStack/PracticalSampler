Engine::getMilliSecondsForTempo(int tempoIndex) -> Double

Thread safety: SAFE -- pure arithmetic via TempoSyncer, atomic BPM read
Returns duration in milliseconds for a tempo subdivision at the current host BPM.
Index 0=1/1, 5=1/4, 8=1/8, 11=1/16, etc.
Pair with:
  getTempoName -- get human-readable name for same index
Source:
  ScriptingApi.cpp  Engine::getMilliSecondsForTempo()
    -> TempoSyncer::getTempoInMilliSeconds(getHostBpm(), tempoIndex)
