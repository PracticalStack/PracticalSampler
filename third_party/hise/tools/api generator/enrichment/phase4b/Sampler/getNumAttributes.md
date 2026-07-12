Sampler::getNumAttributes() -> Integer

Thread safety: SAFE
Returns the total number of processor parameters (attributes) available on this
sampler. Includes both base ModulatorSynth parameters and sampler-specific ones.
Pair with:
  getAttribute -- read a parameter by index
  setAttribute -- write a parameter by index
Source:
  ScriptingApi.cpp  Sampler::getNumAttributes()
    -> Processor::getNumParameters()
