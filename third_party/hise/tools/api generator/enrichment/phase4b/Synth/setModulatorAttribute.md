Synth::setModulatorAttribute(Integer chainId, Integer modulatorIndex, Integer attributeIndex, Double newValue) -> undefined

Thread safety: UNSAFE -- calls setAttribute or setIntensity on target modulator, then sendOtherChangeMessage.
Sets an attribute on a modulator within the gain or pitch chain by positional index.
Special indices: -12 = Intensity, -13 = Bypassed. For pitch chain intensity, value is in
semitones, converted to ratio via pow(2, value/12.0).

Dispatch/mechanics:
  chainId: 1 = GainModulation (owner->gainChain), 2 = PitchModulation (owner->pitchChain)
  Find modulator at modulatorIndex in chain handler
  attributeIndex -12: Modulation::setIntensity(value) -- for pitch chain, pow(2, value/12.0)
  attributeIndex -13: setBypassed(newValue == 1.0f)
  Other: setAttribute(attributeIndex, newValue, dontSendNotification)
  Then: sendOtherChangeMessage() for UI updates

Anti-patterns:
  - Do NOT use chainId 0 -- valid values are 1 (Gain) and 2 (Pitch).
  - For pitch intensity, value is semitones (-12 to +12), clamped after conversion to 0.5-2.0 ratio.

Source:
  ScriptingApi.cpp  Synth::setModulatorAttribute()
    -> switch(chainId) to get chain -> chain->getHandler()->getProcessor(modulatorIndex)
    -> special handling for attributeIndex -12 (intensity) and -13 (bypass)
