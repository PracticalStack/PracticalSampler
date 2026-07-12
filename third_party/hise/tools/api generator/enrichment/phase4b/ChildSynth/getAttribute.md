ChildSynth::getAttribute(int parameterIndex) -> Double

Thread safety: SAFE
Returns the current value of the attribute at the specified parameter index.
Base ModulatorSynth indices: 0=Gain (0.0-1.0), 1=Balance (-100..100),
2=VoiceLimit, 3=KillFadeTime. Use dynamic constants (cs.Gain) instead of raw numbers.
Pair with:
  setAttribute -- set the value at the same index
  getAttributeId -- get string name for a parameter index
Source:
  ScriptingApiObjects.cpp  getAttribute()
    -> synth->getAttribute(parameterIndex)
