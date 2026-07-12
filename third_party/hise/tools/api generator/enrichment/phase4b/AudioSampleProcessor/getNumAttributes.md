AudioSampleProcessor::getNumAttributes() -> Integer

Thread safety: SAFE
Returns the total number of parameters exposed by the wrapped processor module. Count
depends on the concrete module type (e.g., AudioLooper has 10, ConvolutionEffect has 10).
Dispatch/mechanics:
  audioSampleProcessor->getNumParameters()
Pair with:
  getAttributeId -- iterate parameters by index
  getAttribute -- read parameter values
Source:
  ScriptingApiObjects.cpp:4763+  getNumAttributes() -> Processor::getNumParameters()
