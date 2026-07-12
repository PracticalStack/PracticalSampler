Modulator::getIntensity() -> Double

Thread safety: SAFE
Returns the current modulation intensity. For PitchMode modulators, the internal
normalized value (-1.0..1.0) is converted back to semitones (-12.0..12.0). For
all other modes, returns the raw intensity value.

Dispatch/mechanics:
  PitchMode: return m->getIntensity() * 12.0
  else: return m->getIntensity()

Pair with:
  setIntensity -- set the intensity value

Source:
  ScriptingApiObjects.cpp:3153  getIntensity()
    -> branches on PitchMode for semitone conversion
