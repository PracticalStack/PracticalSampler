Engine::getSamplesForQuarterBeats(Number quarterBeats) -> Double

Thread safety: SAFE -- pure arithmetic, atomic BPM read
Converts quarter beats to samples at the current host BPM.
Pair with:
  getQuarterBeatsForSamples -- inverse
  getSamplesForQuarterBeatsWithTempo -- explicit BPM variant
Source:
  ScriptingApi.cpp  -> getSamplesForQuarterBeatsWithTempo(quarterBeats, getHostBpm())
