Modulator::getNumAttributes() -> Integer

Thread safety: SAFE
Returns the total number of parameters exposed by this modulator. Available
parameters depend on the modulator type (e.g., LFO has Frequency, FadeIn,
TempoSync; AHDSR has Attack, Decay, etc.).

Pair with:
  getAttributeId -- iterate parameter names using 0..getNumAttributes()-1

Source:
  ScriptingApiObjects.cpp  getNumAttributes() -> mod->getNumParameters()
