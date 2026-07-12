Sampler::getAttribute(Number index) -> Number

Thread safety: SAFE
Returns the value of a sampler module attribute (processor parameter) by index.
Includes both base ModulatorSynth parameters and sampler-specific parameters
(PreloadSize, BufferSize, VoiceAmount, RRGroupAmount, etc.).
Pair with:
  setAttribute -- sets a parameter value
  getAttributeId -- gets parameter name from index
  getAttributeIndex -- gets parameter index from name
Source:
  ScriptingApi.cpp  Sampler::getAttribute()
    -> Processor::getAttribute(index)
