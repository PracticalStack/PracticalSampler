ChildSynth::setAttribute(int parameterIndex, float newValue) -> undefined

Thread safety: SAFE -- uses ProcessorHelpers::getAttributeNotificationType() for thread-safe notification dispatching
Sets the value of the attribute at the specified parameter index. Base ModulatorSynth
indices: 0=Gain, 1=Balance, 2=VoiceLimit, 3=KillFadeTime. Use dynamic constants
(cs.Gain, cs.Balance) rather than raw numbers.
Pair with:
  getAttribute -- read back the current value
  getAttributeId / getAttributeIndex -- lookup between names and indices
Source:
  ScriptingApiObjects.cpp  setAttribute()
    -> synth->setAttribute(parameterIndex, newValue, ProcessorHelpers::getAttributeNotificationType())
