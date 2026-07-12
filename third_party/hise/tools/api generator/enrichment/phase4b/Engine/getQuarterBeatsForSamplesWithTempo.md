Engine::getQuarterBeatsForSamplesWithTempo(double samples, double bpm) -> Double

Thread safety: SAFE -- pure arithmetic
Converts sample count to quarter beats at an explicit BPM.
At 120 BPM, 44100 Hz: 22050 samples = 1.0 quarter beat.
Pair with:
  getSamplesForQuarterBeatsWithTempo -- inverse
Source:
  ScriptingApi.cpp  -> samples / TempoSyncer::getTempoInSamples(bpm, sr, Quarter)
