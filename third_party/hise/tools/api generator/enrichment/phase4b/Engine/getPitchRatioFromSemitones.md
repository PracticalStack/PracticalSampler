Engine::getPitchRatioFromSemitones(double semiTones) -> Double

Thread safety: SAFE -- pure inline arithmetic: pow(2.0, semiTones / 12.0)
Converts semitone offset to pitch ratio. 0->1.0, 12->2.0, -12->0.5, 7->~1.498.
Pair with:
  getSemitonesFromPitchRatio -- inverse (NOTE: returns cents, not semitones)
Source:
  ScriptingApi.h  inline -> pow(2.0, semiTones / 12.0)
