ChildSynth::getNumAttributes() -> int

Thread safety: SAFE
Returns the total number of attributes (parameters) on the wrapped synth processor.
Base ModulatorSynth has 4 (Gain, Balance, VoiceLimit, KillFadeTime). Subclasses add more.
Pair with:
  getAttribute / setAttribute -- access parameters by index
  getAttributeId -- get string name for each index
Source:
  ScriptingApiObjects.cpp  getNumAttributes()
    -> synth->getNumParameters()
