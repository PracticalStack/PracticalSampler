Engine::getSemitonesFromPitchRatio(Number pitchRatio) -> Double

Thread safety: SAFE -- pure inline arithmetic
Converts pitch ratio using 1200.0 * log2(pitchRatio).
Anti-patterns:
  - [BUG] Returns cents (1200 per octave), NOT semitones (12 per octave), despite
    the method name. Divide result by 100 for actual semitones.
    getPitchRatioFromSemitones correctly uses semitones, so these are not true inverses.
Source:
  ScriptingApi.h  inline -> 1200.0 * log2(pitchRatio)
