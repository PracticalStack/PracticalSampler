Synth::getAttribute(Integer attributeIndex) -> Double

Thread safety: SAFE -- directly calls owner->getAttribute() which reads a float member, no allocations, no locks.
Returns the value of a parameter on the parent synth by attribute index.
Standard ModulatorSynth indices: 0=Gain (0.0-1.0), 1=Balance (-100 to 100), 2=VoiceLimit, 3=KillFadeTime.

Pair with:
  setAttribute -- set the same parameter by index

Source:
  ScriptingApi.cpp  Synth::getAttribute()
    -> owner->getAttribute(attributeIndex)
