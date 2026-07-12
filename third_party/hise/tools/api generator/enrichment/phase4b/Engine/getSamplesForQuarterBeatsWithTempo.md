Engine::getSamplesForQuarterBeatsWithTempo(Number quarterBeats, Number bpm) -> Double

Thread safety: SAFE -- pure arithmetic
Converts quarter beats to samples at an explicit BPM.
Pair with:
  getQuarterBeatsForSamplesWithTempo -- inverse
Source:
  ScriptingApi.cpp  -> TempoSyncer::getTempoInSamples(bpm, sr, Quarter) * quarterBeats
