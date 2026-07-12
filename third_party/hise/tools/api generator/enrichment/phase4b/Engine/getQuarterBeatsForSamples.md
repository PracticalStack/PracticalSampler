Engine::getQuarterBeatsForSamples(double samples) -> Double

Thread safety: SAFE -- pure arithmetic, atomic BPM read
Converts sample count to quarter beats at the current host BPM.
Pair with:
  getSamplesForQuarterBeats -- inverse
  getQuarterBeatsForSamplesWithTempo -- explicit BPM variant
Source:
  ScriptingApi.cpp  -> getQuarterBeatsForSamplesWithTempo(samples, getHostBpm())
